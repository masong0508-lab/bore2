package com.bore2.game;

import android.app.Activity;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Native 3D neighborhood preview; map mode remains available for navigation. */
public final class MainActivity extends Activity {
    private static final int MAP_SIZE=1024;
    private static final int BG=Color.rgb(23,32,34), INK=Color.rgb(236,242,236);
    private SharedPreferences prefs;
    private TextView title,detail,hint;
    private MapView map;
    private WorldView3D world3d;
    private Button dayButton,modeButton;
    private final ExecutorService meshWorker=Executors.newSingleThreadExecutor();
    private volatile int serial;
    private int meshX=-1,meshY=-1,meshSeed;
    private int seed,capitalX,capitalY,cx,cy,radius=8,selectedX,selectedY,weekday;
    private boolean in3d=true;
    private int[] tiles=new int[0];

    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        prefs=getSharedPreferences("world_preview",MODE_PRIVATE);
        seed=prefs.getInt("seed",1234);
        radius=Math.max(4,Math.min(16,prefs.getInt("radius",8)));
        weekday=prefs.getBoolean("weekend",false)?5:0;
        in3d=prefs.getBoolean("view3d",true);
        resetNative();
        cx=clampCenter(prefs.getInt("x",capitalX));cy=clampCenter(prefs.getInt("y",capitalY));
        selectedX=cx;selectedY=cy;

        boolean wide=getResources().getConfiguration().orientation==Configuration.ORIENTATION_LANDSCAPE;
        LinearLayout root=new LinearLayout(this);
        root.setOrientation(wide?LinearLayout.HORIZONTAL:LinearLayout.VERTICAL);
        root.setBackgroundColor(BG);
        root.setOnApplyWindowInsetsListener((v,insets)->{
            if(Build.VERSION.SDK_INT>=30){
                android.graphics.Insets p=insets.getInsets(WindowInsets.Type.systemBars());
                v.setPadding(p.left,p.top,p.right,p.bottom);
            }else v.setPadding(insets.getSystemWindowInsetLeft(),insets.getSystemWindowInsetTop(),
                               insets.getSystemWindowInsetRight(),insets.getSystemWindowInsetBottom());
            return insets;
        });
        LinearLayout panel=new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);panel.setBackgroundColor(BG);
        FrameLayout pane=new FrameLayout(this);
        pane.setBackgroundColor(Color.rgb(174,196,204));
        if(wide){
            root.addView(pane,new LinearLayout.LayoutParams(0,-1,1));
            root.addView(panel,new LinearLayout.LayoutParams(dp(260),-1));
        }else{
            root.addView(panel,new LinearLayout.LayoutParams(-1,-2));
            root.addView(pane,new LinearLayout.LayoutParams(-1,0,1));
        }
        title=new TextView(this);title.setTextSize(18);title.setTextColor(INK);
        title.setPadding(dp(12),dp(8),dp(12),dp(2));panel.addView(title);
        hint=new TextView(this);hint.setTextColor(Color.rgb(180,201,190));hint.setTextSize(12);
        hint.setPadding(dp(12),0,dp(12),dp(6));panel.addView(hint);
        world3d=new WorldView3D(this,(x,y,ox,oy)->{
            if(!in3d||ox!=cx||oy!=cy||meshSeed!=seed||meshX!=cx||meshY!=cy)return;
            selectedX=x;selectedY=y;updateDetails();
        });
        map=new MapView();
        pane.addView(world3d,new FrameLayout.LayoutParams(-1,-1));
        pane.addView(map,new FrameLayout.LayoutParams(-1,-1));
        LinearLayout lower=wide?panel:new LinearLayout(this);
        lower.setOrientation(LinearLayout.VERTICAL);
        if(!wide)root.addView(lower,new LinearLayout.LayoutParams(-1,-2));
        detail=new TextView(this);detail.setTextSize(14);detail.setTextColor(INK);
        detail.setMinLines(2);detail.setPadding(dp(12),dp(6),dp(12),dp(4));
        lower.addView(detail,new LinearLayout.LayoutParams(-1,-2));
        LinearLayout first=new LinearLayout(this);first.setGravity(Gravity.CENTER);
        lower.addView(first,new LinearLayout.LayoutParams(-1,-2));
        modeButton=addButton(first,in3d?"Map":"3D",()->{
            in3d=!in3d;showMode();if(in3d)requestScene();else map.invalidate();
        });
        addButton(first,"Zoom −",()->{if(in3d)world3d.zoomBy(1.22f);else{radius=Math.min(16,radius+2);refresh();}});
        addButton(first,"Zoom +",()->{if(in3d)world3d.zoomBy(.82f);else{radius=Math.max(4,radius-2);refresh();}});
        addButton(first,"Center lot",()->{cx=selectedX;cy=selectedY;refresh();});
        LinearLayout second=new LinearLayout(this);second.setGravity(Gravity.CENTER);
        lower.addView(second,new LinearLayout.LayoutParams(-1,-2));
        addButton(second,"Capital",()->{cx=capitalX;cy=capitalY;selectedX=cx;selectedY=cy;refresh();});
        dayButton=addButton(second,weekday==5?"Weekend":"Weekday",()->{
            weekday=weekday==5?0:5;dayButton.setText(weekday==5?"Weekend":"Weekday");updateDetails();
        });
        addButton(second,"New seed",()->{
            serial++;meshX=meshY=-1;seed=(int)System.nanoTime();resetNative();
            cx=capitalX;cy=capitalY;selectedX=cx;selectedY=cy;world3d.resetCamera();refresh();
        });
        setContentView(root);root.requestApplyInsets();showMode();refresh();
    }
    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
    private Button addButton(LinearLayout row,String label,Runnable action){
        Button b=new Button(this);b.setText(label);b.setAllCaps(false);b.setTextSize(12);
        b.setMinWidth(0);b.setMinimumWidth(0);b.setPadding(0,0,0,0);
        row.addView(b,new LinearLayout.LayoutParams(0,dp(44),1));
        b.setOnClickListener(v->action.run());return b;
    }
    private void resetNative(){
        int center=NativeWorld.init(seed);capitalX=center&65535;capitalY=center>>>16;
    }
    private int clampCenter(int x){return Math.max(radius,Math.min(MAP_SIZE-radius-1,x));}
    private void showMode(){
        world3d.setVisibility(in3d?View.VISIBLE:View.GONE);map.setVisibility(in3d?View.GONE:View.VISIBLE);
        modeButton.setText(in3d?"Map":"3D");
        hint.setText(in3d?"Drag to orbit · Pinch to zoom · Tap a lot":"Drag to explore · Tap a lot · Center lot to travel");
        world3d.select(selectedX,selectedY);
    }
    private void refresh(){
        cx=clampCenter(cx);cy=clampCenter(cy);
        int[] next=NativeWorld.tiles(cx,cy,radius);
        int side=2*radius+1;
        tiles=next!=null&&next.length==side*side?next:new int[0];
        title.setText("BORE 2  /  SAN BOREAS   "+cx+", "+cy);
        if(selectedX<cx-radius||selectedX>cx+radius||selectedY<cy-radius||selectedY>cy+radius){
            selectedX=cx;selectedY=cy;
        }
        updateDetails();map.invalidate();if(in3d)requestScene();
    }
    private void requestScene(){
        if(meshX==cx&&meshY==cy&&meshSeed==seed){
            world3d.select(selectedX,selectedY);return;
        }
        final int version=++serial,x=cx,y=cy,key=seed;
        meshX=meshY=-1;
        hint.setText("Loading 3D neighborhood…");
        meshWorker.execute(()->{
            if(version!=serial)return;
            float[] mesh=NativeWorld.mesh(x,y,4);
            if(version!=serial)return;
            runOnUiThread(()->{
                if(version!=serial||isFinishing()||isDestroyed())return;
                if(mesh==null||mesh.length==0){
                    in3d=false;showMode();hint.setText("3D scene unavailable — map remains usable");return;
                }
                meshX=x;meshY=y;meshSeed=key;world3d.setScene(mesh,x,y);world3d.select(selectedX,selectedY);
                if(in3d)hint.setText("Drag to orbit · Pinch to zoom · Tap a lot");
            });
        });
    }
    private void updateDetails(){
        String text=NativeWorld.describe(selectedX,selectedY,weekday);
        detail.setText(text+"\nBlock "+selectedX+", "+selectedY+"  ·  Seed "+Integer.toUnsignedString(seed));
        map.setContentDescription("San Boreas map. Selected lot: "+text);
        world3d.setContentDescription("San Boreas 3D neighborhood. Selected lot: "+text);
        world3d.select(selectedX,selectedY);
    }
    @Override protected void onResume(){super.onResume();if(world3d!=null)world3d.onResume();}
    @Override protected void onPause(){
        prefs.edit().putInt("seed",seed).putInt("x",cx).putInt("y",cy)
             .putInt("radius",radius).putBoolean("weekend",weekday==5).putBoolean("view3d",in3d).apply();
        if(world3d!=null)world3d.onPause();super.onPause();
    }
    @Override protected void onDestroy(){serial++;meshWorker.shutdownNow();super.onDestroy();}
    private final class MapView extends View {
        private final Paint paint=new Paint();
        private final int[] terrain={0xff2a6475,0xffb6a075,0xff76915d,0xff416c50,0xffb89b68,0xff777a73};
        private float startX,startY,offsetX,offsetY;
        private boolean down,drag;
        MapView(){super(MainActivity.this);setFocusable(true);setClickable(true);}
        private float size(){return Math.min(getWidth(),getHeight());}
        private float cell(){return size()/(radius*2+1);}
        private float left(){return (getWidth()-size())/2f;}
        private float top(){return (getHeight()-size())/2f;}
        @Override protected void onDraw(Canvas c){
            c.drawColor(BG);
            int side=2*radius+1;float step=cell();
            if(step<=0||tiles.length!=side*side)return;
            float lx=left(),ly=top();
            c.save();c.clipRect(lx,ly,lx+size(),ly+size());
            paint.setStyle(Paint.Style.FILL);paint.setAntiAlias(false);
            for(int iy=0;iy<side;iy++)for(int ix=0;ix<side;ix++){
                int code=tiles[iy*side+ix],land=code&7,lot=(code>>>3)&63;
                int road=(code>>>9)&255,floors=(code>>>17)&7;
                float x=lx+ix*step+offsetX,y=ly+iy*step+offsetY;
                paint.setColor(terrain[Math.min(terrain.length-1,land)]);
                c.drawRect(x,y,x+step+0.5f,y+step+0.5f,paint);
                if(lot>0&&lot!=19&&land!=0){
                    float margin=step*.20f;
                    if(lot<=5)paint.setColor(0xffe4c8a0);
                    else if(lot==16||lot==18||lot==20)paint.setColor(0xffaccb83);
                    else if(lot==32)paint.setColor(0xff478a9f);
                    else paint.setColor(0xffca8f72);
                    c.drawRect(x+margin,y+margin,x+step*.90f,y+step*.90f,paint);
                    if(floors>1){paint.setColor(0xff71584c);
                        c.drawRect(x+step*.64f,y+margin,x+step*.9f,y+step*.90f,paint);}
                }
                paint.setColor(0xff434b4a);
                if((road&1)!=0)c.drawRect(x,y,x+step,y+step*.18f,paint);
                if((road&2)!=0)c.drawRect(x,y,x+step*.18f,y+step,paint);
            }
            int sx=selectedX-cx+radius,sy=selectedY-cy+radius;
            if(sx>=0&&sy>=0&&sx<side&&sy<side){
                paint.setColor(0xfff6da75);paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(Math.max(2,step*.1f));
                float x=lx+sx*step+offsetX,y=ly+sy*step+offsetY;
                c.drawRect(x+1,y+1,x+step-1,y+step-1,paint);
                paint.setStyle(Paint.Style.FILL);
            }
            c.restore();
        }
        @Override public boolean onTouchEvent(MotionEvent event){
            float x=event.getX(),y=event.getY();
            switch(event.getActionMasked()){
                case MotionEvent.ACTION_DOWN:
                    down=true;drag=false;startX=x;startY=y;offsetX=offsetY=0;return true;
                case MotionEvent.ACTION_MOVE:
                    if(!down)return false;
                    float dx=x-startX,dy=y-startY;
                    if(Math.abs(dx)>dp(6)||Math.abs(dy)>dp(6))drag=true;
                    if(drag){offsetX=dx;offsetY=dy;invalidate();}
                    return true;
                case MotionEvent.ACTION_UP:
                    if(!down)return false;down=false;
                    float step=cell();
                    if(step>0&&drag){
                        cx-=Math.round((x-startX)/step);cy-=Math.round((y-startY)/step);
                        selectedX=clampCenter(cx);selectedY=clampCenter(cy);
                        offsetX=offsetY=0;refresh();
                    }else if(step>0&&x>=left()&&y>=top()&&x<left()+size()&&y<top()+size()){
                        selectedX=cx-radius+(int)((x-left())/step);
                        selectedY=cy-radius+(int)((y-top())/step);
                        offsetX=offsetY=0;updateDetails();performClick();invalidate();
                    }
                    return true;
                case MotionEvent.ACTION_CANCEL:
                    down=false;offsetX=offsetY=0;invalidate();return true;
                default:return true;
            }
        }
        @Override public boolean performClick(){super.performClick();return true;}
    }
}
