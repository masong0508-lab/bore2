package com.bore2.game;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Color;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;

/** Interactive cutaway home. Only walkable routes can trigger furniture actions. */
final class InteriorView extends View {
    interface Listener {void arrived(int action,int cell);void message(String message);}
    static final int WALK=-1,EXIT=-2;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Listener listener;
    private InteriorPlan plan;
    private int at,action=WALK,index,lo,hi,top,bottom;
    private int[] path;
    private long started;
    private float tile,left,upper;
    private static final long STEP_MS=145;
    private final Runnable tick=this::step;
    InteriorView(Context context,Listener listener){
        super(context);this.listener=listener;setClickable(true);setFocusable(true);
        setContentDescription("Home cutaway. Tap a floor tile to walk or furniture to interact.");
    }
    void open(InteriorPlan p,int saved){
        cancel();plan=p;at=p.walkable(saved)?saved:p.entry;
        lo=18;hi=-1;top=18;bottom=-1;
        for(int i=0;i<InteriorPlan.N;i++)if(p.floor(i)){lo=Math.min(lo,i%18);hi=Math.max(hi,i%18);top=Math.min(top,i/18);bottom=Math.max(bottom,i/18);}
        invalidate();
    }
    int cell(){return at;}
    boolean busy(){return path!=null;}
    void cancel(){removeCallbacks(tick);path=null;index=0;action=WALK;invalidate();}
    void close(){cancel();plan=null;}
    void use(int kind){
        if(plan==null)return;
        begin(plan.routeToObject(at,kind),kind);
    }
    void exit(){if(plan!=null)begin(plan.route(at,plan.entry),EXIT);}
    private void begin(int[] next,int what){
        if(next==null){listener.message("No clear path");return;}
        cancel();action=what;
        if(next.length==0){finish();return;}
        path=next;started=SystemClock.uptimeMillis();postOnAnimation(tick);invalidate();
    }
    private void finish(){
        int done=action;path=null;action=WALK;index=0;
        if(plan!=null&&(done==WALK||(done==EXIT&&at==plan.entry)||(done>=0&&plan.canUse(at,done))))listener.arrived(done,at);
        invalidate();
    }
    private void step(){
        if(path==null||plan==null)return;
        long now=SystemClock.uptimeMillis();
        while(path!=null&&now-started>=STEP_MS){
            int next=path[index];
            if(!plan.canStep(at,next)){cancel();listener.message("Path blocked");return;}
            at=next;index++;started+=STEP_MS;
            if(index==path.length){finish();return;}
        }
        invalidate();if(path!=null)postOnAnimation(tick);
    }
    @Override protected void onDetachedFromWindow(){cancel();super.onDetachedFromWindow();}
    private float X(float x){return left+(x-lo)*tile;}
    private float Y(float y){return upper+(y-top)*tile;}
    private void rect(Canvas c,float x,float y,float w,float h,int color){
        paint.setColor(color);paint.setStyle(Paint.Style.FILL);c.drawRoundRect(X(x),Y(y),X(x+w),Y(y+h),tile*.07f,tile*.07f,paint);
    }
    private void text(Canvas c,String label,float x,float y,float size,int color){
        paint.setColor(color);paint.setTextSize(size);paint.setTextAlign(Paint.Align.CENTER);c.drawText(label,x,y,paint);
    }
    private void furniture(Canvas c,int kind,int cell){
        float x=cell%18,y=cell/18;
        rect(c,x+.08f,y+.10f,.85f,.85f,0x55312218);
        switch(kind){
            case InteriorPlan.BED:
                rect(c,x+.08f,y+.05f,.78f,.85f,0xff805a43);rect(c,x+.14f,y+.11f,.66f,.73f,0xff82a6a3);
                rect(c,x+.20f,y+.15f,.54f,.19f,0xfff4e9cf);break;
            case InteriorPlan.KITCHEN:
                rect(c,x+.05f,y+.10f,.90f,.68f,0xff667e84);rect(c,x+.10f,y+.18f,.37f,.44f,0xffa9d0cd);
                rect(c,x+.53f,y+.18f,.35f,.44f,0xff333a40);rect(c,x+.60f,y+.24f,.20f,.30f,0xff818e8e);break;
            case InteriorPlan.HOBBY:
                rect(c,x+.08f,y+.09f,.84f,.62f,0xffa86f42);rect(c,x+.22f,y+.17f,.48f,.27f,0xffeee1bb);
                rect(c,x+.36f,y+.72f,.28f,.22f,0xff4b696b);break;
            case InteriorPlan.PHONE:
                rect(c,x+.18f,y+.18f,.64f,.63f,0xffb08b64);rect(c,x+.33f,y+.25f,.32f,.46f,0xff263b40);
                rect(c,x+.38f,y+.30f,.22f,.22f,0xff8bbdc6);break;
            case InteriorPlan.SOFA:
                rect(c,x+.08f,y+.11f,.84f,.72f,0xffa9654b);rect(c,x+.17f,y+.23f,.66f,.48f,0xffd89e71);
                rect(c,x+.08f,y+.11f,.84f,.15f,0xff8b523d);break;
            default:break;
        }
    }
    @Override protected void onDraw(Canvas c){
        c.drawColor(0xff223335);if(plan==null)return;
        float pad=Math.max(10,getResources().getDisplayMetrics().density*12);
        tile=Math.min((getWidth()-pad*2)/(hi-lo+1f),(getHeight()-pad*2)/(bottom-top+1f));
        if(tile<=0)return;
        left=(getWidth()-tile*(hi-lo+1))/2;upper=(getHeight()-tile*(bottom-top+1))/2;
        for(int i=0;i<InteriorPlan.N;i++)if(plan.floor(i)){
            int x=i%18,y=i/18;paint.setStyle(Paint.Style.FILL);
            paint.setColor(((x+y)&1)==0?0xffd8c19c:0xffd0b58d);
            c.drawRect(X(x),Y(y),X(x+1)+.25f,Y(y+1)+.25f,paint);
        }
        paint.setStrokeWidth(Math.max(2,tile*.13f));paint.setStrokeCap(Paint.Cap.SQUARE);
        for(int i=0;i<InteriorPlan.N;i++)if(plan.floor(i)){
            int x=i%18,y=i/18;
            for(int d=0;d<4;d++)if((plan.walls[i]&(1<<d))!=0){
                if(i==plan.entry&&!plan.floor(InteriorPlan.neighbor(i,d))){
                    paint.setColor(0xff4fada0);
                }else paint.setColor(0xff56473d);
                if(d==0)c.drawLine(X(x+1),Y(y),X(x+1),Y(y+1),paint);
                else if(d==1)c.drawLine(X(x),Y(y+1),X(x+1),Y(y+1),paint);
                else if(d==2)c.drawLine(X(x),Y(y),X(x),Y(y+1),paint);
                else c.drawLine(X(x),Y(y),X(x+1),Y(y),paint);
            }
        }
        if(path!=null){
            paint.setColor(0xff688f80);for(int k=index;k<path.length;k++)c.drawCircle(X(path[k]%18+.5f),Y(path[k]/18+.5f),tile*.08f,paint);
        }
        for(int k=0;k<InteriorPlan.COUNT;k++)furniture(c,k,plan.objects[k]);
        float px=at%18+.5f,py=at/18+.5f,phase=0;
        if(path!=null){
            float t=Math.min(1f,(SystemClock.uptimeMillis()-started)/(float)STEP_MS);
            int next=path[index];px+=(next%18-at%18)*t;py+=(next/18-at/18)*t;
            phase=(float)Math.sin((index+t)*Math.PI);
        }
        float cx=X(px),cy=Y(py);
        paint.setColor(0x55302010);c.drawOval(cx-tile*.25f,cy-tile*.15f,cx+tile*.25f,cy+tile*.2f,paint);
        paint.setStrokeWidth(Math.max(2,tile*.10f));paint.setStrokeCap(Paint.Cap.ROUND);paint.setColor(0xff2a3949);
        c.drawLine(cx-tile*.10f,cy+tile*.11f,cx-tile*.12f,cy+tile*(.26f+phase*.09f),paint);
        c.drawLine(cx+tile*.10f,cy+tile*.11f,cx+tile*.12f,cy+tile*(.26f-phase*.09f),paint);
        paint.setColor(0xff297c78);c.drawRoundRect(cx-tile*.22f,cy-tile*.18f,cx+tile*.22f,cy+tile*.14f,tile*.13f,tile*.13f,paint);
        paint.setColor(0xffdfb38e);c.drawCircle(cx,cy-tile*.25f,tile*.16f,paint);
        paint.setColor(0xff4d392c);c.drawArc(cx-tile*.17f,cy-tile*.42f,cx+tile*.17f,cy-tile*.11f,180,180,true,paint);
        if(tile>=27){
            for(int k=0;k<InteriorPlan.COUNT;k++){
                int p=plan.objects[k];text(c,InteriorPlan.LABEL[k],X(p%18+.5f),Y(p/18+.98f),Math.min(tile*.22f,14),0xff332f2b);
            }
        }
    }
    @Override public boolean onTouchEvent(MotionEvent event){
        if(event.getActionMasked()!=MotionEvent.ACTION_UP)return true;
        if(plan==null||tile<=0)return true;
        int x=(int)Math.floor((event.getX()-left)/tile)+lo,y=(int)Math.floor((event.getY()-upper)/tile)+top;
        if(!InteriorPlan.inside(x,y))return true;
        int cell=y*18+x;if(!plan.floor(cell))return true;
        performClick();if(plan.occupied[cell]>=0)use(plan.occupied[cell]);else begin(plan.route(at,cell),WALK);
        return true;
    }
    @Override public boolean performClick(){super.performClick();return true;}
}
