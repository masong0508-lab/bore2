package com.bore2.game;

import android.app.Activity;
import android.app.AlertDialog;
import android.widget.Toast;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ScrollView;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Mobile neighborhood life: native world generation, local travel, housing and venue gameplay. */
public final class MainActivity extends Activity {
    private static final int MAP_SIZE=1024;
    private static final int BG=Color.rgb(23,32,34), INK=Color.rgb(236,242,236);
    private SharedPreferences prefs;
    private AvatarStyle appearance=AvatarStyle.defaults();
    private String playerName="Bore";
    private TextView title,detail,hint,lifeStatus;
    private LinearLayout root,panel;
    private FrameLayout pane;
    private MapView map;
    private WorldView3D world3d;
    private LinearLayout homePane;
    private InteriorView interiorView;
    private InteriorPlan interiorPlan;
    private BuildingPlan buildingPlan;
    private HomeDecor homeDecor;
    private String savedDecor="";
    private boolean decorating;
    private LinearLayout decorRow;
    private Button decorateButton,nextSpotButton,themeButton,resetRoomButton,householdButton;
    private int roomFloor=-1;
    private LinearLayout stairsRow;
    private Button upstairsButton,downstairsButton;
    private TextView roomCaption;
    private final Button[] roomButtons=new Button[InteriorPlan.COUNT];
    private Button exitRoomButton;
    private int interiorX=-1,interiorY=-1;
    private boolean inInterior;
    private Button dayButton,modeButton,travelButton,walkButton;
    private boolean walkMode;
    private LinearLayout walkControls;
    private int tileX,tileY,walkIndex,walkDone;
    private int[] outdoorRoute;
    private OutdoorPath outdoorMap;
    private final Runnable outdoorTick=this::stepOutdoor;
    private static final int WALK_MS=160;
    private final Handler travelHandler=new Handler(Looper.getMainLooper());
    private int[] travelRoute;
    private int travelIndex,travelCost,travelElapsed;
    private final Runnable travelTick=this::stepTravel;
    private final ExecutorService meshWorker=Executors.newSingleThreadExecutor();
    private volatile int serial;
    private int meshX=-1,meshY=-1,meshSeed,meshRevision=-1;
    private int requestX=-1,requestY=-1,requestSeed;
    private int seed,capitalX,capitalY,cx,cy,radius=8,selectedX,selectedY,weekday;
    private boolean in3d=true,resumed,crowdMotion=true;
    private Button crowdButton;
    private int[] tiles=new int[0];
    private int lifeX,lifeY,minute=600,funds=250,energy=100,fun=50,social=50,food=100,sessions;
    private int lifeDay,lastShiftDay=-1;
    private int homeX=-1,homeY=-1,rentDebt,careerShifts,trips,homeNights;
    private final MobileContacts contacts=new MobileContacts();
    private final Household household=new Household();
    private ResidentSchedule.Grid residentGrid;
    private int residentGridSeed, residentRevision=-1,residentDay=-1,residentTime=-1,residentHome=-2,residentRoommate=-2;
    private int rentedKey(){return hasHome()?LifeRules.pack(homeX,homeY):-1;}
    private boolean hasHome(){return LifeRules.inside(homeX,homeY);}
    private boolean atHome(){return hasHome()&&lifeX==homeX&&lifeY==homeY;}
    private boolean atSelected(){return lifeX==selectedX&&lifeY==selectedY;}
    private int visualX(){return travelRoute!=null&&travelIndex>0?LifeRules.x(travelRoute[travelIndex-1]):lifeX;}
    private int visualY(){return travelRoute!=null&&travelIndex>0?LifeRules.y(travelRoute[travelIndex-1]):lifeY;}
    private int[] lastRoute=new int[0];
    private final int[] skills=new int[7];
    private static final String[] SKILL_NAMES={"Play","Logic","Fitness","Creativity","Cooking","Farming","Charisma"};
    private static int clamp100(int n){return Math.max(0,Math.min(100,n));}
    private static int clampBlock(int n){return Math.max(0,Math.min(MAP_SIZE-1,n));}

    @Override public void onCreate(Bundle state){
        super.onCreate(state);
        prefs=getSharedPreferences("world_preview",MODE_PRIVATE);
        seed=prefs.getInt("seed",1234);
        appearance=AvatarStyle.decode(prefs.getInt("avatarStyle",-1));playerName=AvatarStyle.cleanName(prefs.getString("avatarName","Bore"));
        radius=Math.max(4,Math.min(16,prefs.getInt("radius",8)));
        weekday=Math.max(0,Math.min(6,prefs.getInt("day",prefs.getBoolean("weekend",false)?5:0)));
        in3d=prefs.getBoolean("view3d",true);crowdMotion=prefs.getBoolean("crowdMotion",true);
        resetNative();
        cx=clampCenter(prefs.getInt("x",capitalX));cy=clampCenter(prefs.getInt("y",capitalY));
        selectedX=cx;selectedY=cy;
        lifeX=clampBlock(prefs.getInt("lifeX",capitalX));lifeY=clampBlock(prefs.getInt("lifeY",capitalY));
        tileX=prefs.getInt("tileX",lifeX*18+2);tileY=prefs.getInt("tileY",lifeY*18+2);
        if(!OutdoorPath.valid(tileX,tileY)||tileX/18!=lifeX||tileY/18!=lifeY)resetOutdoorPosition();
        minute=Math.max(0,Math.min(1439,prefs.getInt("minute",600)));
        funds=LifeRules.credit(prefs.getInt("funds",250),0);energy=clamp100(prefs.getInt("energy",100));
        fun=clamp100(prefs.getInt("fun",50));social=clamp100(prefs.getInt("social",50));food=clamp100(prefs.getInt("food",100));
        sessions=Math.max(0,prefs.getInt("sessions",0));
        lifeDay=LifeRules.credit(prefs.getInt("lifeDay",0),0);lastShiftDay=prefs.getInt("lastShiftDay",-1);
        homeX=prefs.getInt("homeX",-1);homeY=prefs.getInt("homeY",-1);
        if(!hasHome()){homeX=homeY=-1;}
        rentDebt=LifeRules.credit(prefs.getInt("rentDebt",0),0);
        careerShifts=LifeRules.credit(prefs.getInt("careerShifts",0),0);
        trips=LifeRules.credit(prefs.getInt("trips",0),0);homeNights=LifeRules.credit(prefs.getInt("homeNights",0),0);
        for(int i=0;i<skills.length;i++)skills[i]=Math.max(0,Math.min(4400,prefs.getInt("skill"+i,0)));
        contacts.load(prefs.getString("contacts",""));savedDecor=prefs.getString("homeDecor","");
        String savedHousehold=prefs.getString("household","");
        int previousMember=Household.savedMember(savedHousehold,seed,rentedKey());
        if(previousMember>=0)household.load(savedHousehold,seed,rentedKey(),contacts.affinity(previousMember),validRoommateResidence(previousMember));

        root=new LinearLayout(this);
        root.setBackgroundColor(BG);
        root.setOnApplyWindowInsetsListener((v,insets)->{
            if(Build.VERSION.SDK_INT>=30){
                android.graphics.Insets p=insets.getInsets(WindowInsets.Type.systemBars());
                v.setPadding(p.left,p.top,p.right,p.bottom);
            }else v.setPadding(insets.getSystemWindowInsetLeft(),insets.getSystemWindowInsetTop(),
                               insets.getSystemWindowInsetRight(),insets.getSystemWindowInsetBottom());
            return insets;
        });
        panel=new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);panel.setBackgroundColor(BG);
        pane=new FrameLayout(this);
        pane.setBackgroundColor(Color.rgb(174,196,204));
        // Keep the GLSurfaceView attached across rotation. Change only
        // the two pane layout parameters, not the Activity or GL renderer.
        root.addView(pane);root.addView(panel);
        layoutPanes(getResources().getConfiguration());
        title=new TextView(this);title.setTextSize(18);title.setTextColor(INK);
        title.setPadding(dp(12),dp(8),dp(12),dp(2));panel.addView(title);
        hint=new TextView(this);hint.setTextColor(Color.rgb(180,201,190));hint.setTextSize(12);
        hint.setPadding(dp(12),0,dp(12),dp(6));panel.addView(hint);
        world3d=new WorldView3D(this,(x,y,tx,ty,ox,oy,id)->{
            if(inInterior||travelRoute!=null||!in3d||id!=serial||id!=meshRevision||ox!=cx||oy!=cy||meshSeed!=seed||meshX!=cx||meshY!=cy)return;
            if(walkMode){walkTo(tx,ty);return;}
            selectedX=x;selectedY=y;updateDetails();
        });
        world3d.appearance(appearance);world3d.residentSeed(seed);
        map=new MapView();
        pane.addView(world3d,new FrameLayout.LayoutParams(-1,-1));
        pane.addView(map,new FrameLayout.LayoutParams(-1,-1));
        buildWalkControls();buildHomePane();
        ScrollView scroll=new ScrollView(this);
        scroll.setFillViewport(false);
        panel.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        LinearLayout lower=new LinearLayout(this);
        lower.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(lower,new ScrollView.LayoutParams(-1,-2));
        LinearLayout primary=new LinearLayout(this);primary.setGravity(Gravity.CENTER);
        lower.addView(primary,new LinearLayout.LayoutParams(-1,-2));
        travelButton=addButton(primary,"Travel here",this::visitSelected);
        addButton(primary,"Home",this::homeAction);
        addButton(primary,"Find places",this::showFinder);
        addButton(primary,"Enter",this::enterSelected);
        addButton(primary,"Character",this::showCharacterEditor);
        detail=new TextView(this);detail.setTextSize(14);detail.setTextColor(INK);
        detail.setMinLines(2);detail.setPadding(dp(12),dp(6),dp(12),dp(4));
        lower.addView(detail,new LinearLayout.LayoutParams(-1,-2));
        lifeStatus=new TextView(this);lifeStatus.setTextSize(12);lifeStatus.setTextColor(INK);
        lifeStatus.setPadding(dp(12),0,dp(12),dp(4));lower.addView(lifeStatus);
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
        dayButton=addButton(second,"Sleep",()->{if(atHome())enterHome(-1);else notice("Travel home and use the bed");});
        addButton(second,"New life",()->new AlertDialog.Builder(this).setTitle("Start a new life?")
            .setMessage("This replaces your current world, home, savings and progress on this device.")
            .setPositiveButton("Start over",(dialog,which)->newLife()).setNegativeButton("Keep playing",null).show());
        LinearLayout play=new LinearLayout(this);play.setGravity(Gravity.CENTER);
        lower.addView(play,new LinearLayout.LayoutParams(-1,-2));
        addButton(play,"Activities",this::showActivities);
        addButton(play,"Rest 1h",this::rest);
        addButton(play,"Meet locals",this::showLocals);
        LinearLayout work=new LinearLayout(this);work.setGravity(Gravity.CENTER);
        lower.addView(work,new LinearLayout.LayoutParams(-1,-2));
        addButton(work,"Work shift",this::workShift);
        addButton(work,"Eat / cook",()->{if(atHome())enterHome(-1);else eatMeal();});
        addButton(work,"My location",()->{cx=lifeX;cy=lifeY;selectedX=lifeX;selectedY=lifeY;refresh();});
        LinearLayout progress=new LinearLayout(this);progress.setGravity(Gravity.CENTER);
        lower.addView(progress,new LinearLayout.LayoutParams(-1,-2));
        addButton(progress,"Contacts",this::showContactList);
        addButton(progress,"Life goals",this::showGoals);
        crowdButton=addButton(progress,crowdMotion?"Crowds: moving":"Crowds: still",this::toggleCrowds);
        setContentView(root);root.requestApplyInsets();showMode();refresh();
        if(atHome()&&prefs.getBoolean("insideHome",false))enterHome(prefs.getInt("roomCell",-1),prefs.getInt("roomFloor",0));
        else if(prefs.getBoolean("insideVenue",false)&&prefs.getInt("insideX",-1)==lifeX&&prefs.getInt("insideY",-1)==lifeY){
            selectedX=lifeX;selectedY=lifeY;
            enterVenue(prefs.getInt("roomCell",-1),-1,prefs.getInt("insideLot",-1),prefs.getInt("roomFloor",0));
        }
    }
    private void newLife(){
            if(inInterior)leaveHome();
            serial++;meshX=meshY=meshRevision=requestX=requestY=-1;seed=(int)System.nanoTime();resetNative();
            cx=capitalX;cy=capitalY;selectedX=cx;selectedY=cy;
            lifeX=cx;lifeY=cy;resetOutdoorPosition();weekday=0;minute=600;funds=250;energy=food=100;fun=social=50;sessions=0;
            lifeDay=0;lastShiftDay=-1;lastRoute=new int[0];savedDecor="";homeDecor=null;household.clear();homeX=homeY=-1;rentDebt=careerShifts=trips=homeNights=0;
            java.util.Arrays.fill(skills,0);contacts.load("");world3d.residentSeed(seed);world3d.resetCamera();refresh();saveLife();
    }
    private void layoutPanes(Configuration config){
        boolean wide=config.orientation==Configuration.ORIENTATION_LANDSCAPE;
        root.setOrientation(wide?LinearLayout.HORIZONTAL:LinearLayout.VERTICAL);
        pane.setLayoutParams(wide?new LinearLayout.LayoutParams(0,-1,3):new LinearLayout.LayoutParams(-1,0,3));
        panel.setLayoutParams(wide?new LinearLayout.LayoutParams(0,-1,2):new LinearLayout.LayoutParams(-1,0,2));
    }
    @Override public void onConfigurationChanged(Configuration config){
        super.onConfigurationChanged(config);
        if(root!=null){layoutPanes(config);root.requestApplyInsets();root.requestLayout();}
        if(world3d!=null&&in3d)world3d.requestRender();
    }
    private int dp(int value){return Math.round(value*getResources().getDisplayMetrics().density);}
    private Button addButton(LinearLayout row,String label,Runnable action){
        Button b=new Button(this);b.setText(label);b.setAllCaps(false);b.setTextSize(12);
        b.setMinWidth(0);b.setMinimumWidth(0);b.setPadding(0,0,0,0);
        row.addView(b,new LinearLayout.LayoutParams(0,dp(44),1));
        b.setOnClickListener(v->{if(travelRoute!=null&&b!=travelButton){notice("Finish or cancel your trip first");return;}if(inInterior){notice("Use a station or leave the building first");return;}if(walkMode&&b!=walkButton){if(outdoorRoute!=null){notice("Finish or stop walking first");return;}endWalkMode(false);}action.run();});return b;
    }
    private void resetNative(){
        int center=NativeWorld.init(seed);capitalX=center&65535;capitalY=center>>>16;
    }
    private int clampCenter(int x){return Math.max(radius,Math.min(MAP_SIZE-radius-1,x));}
    private void toggleCrowds(){crowdMotion=!crowdMotion;crowdButton.setText(crowdMotion?"Crowds: moving":"Crowds: still");world3d.setCrowdActive(resumed&&in3d&&!inInterior&&crowdMotion);saveLife();}
    private void showMode(){
        world3d.setCrowdActive(resumed&&in3d&&!inInterior&&crowdMotion);
        walkButton.setVisibility(in3d&&!inInterior?View.VISIBLE:View.GONE);
        walkControls.setVisibility(walkMode&&!inInterior?View.VISIBLE:View.GONE);
        if(inInterior){world3d.setVisibility(View.GONE);map.setVisibility(View.GONE);homePane.setVisibility(View.VISIBLE);hint.setText(interiorPlan.commercial?"Venue · walk to a service station":"Home · tap a floor tile or use furniture");return;}
        homePane.setVisibility(View.GONE);
        world3d.setVisibility(in3d?View.VISIBLE:View.GONE);map.setVisibility(in3d?View.GONE:View.VISIBLE);
        modeButton.setText(in3d?"Map":"3D");
        hint.setText(walkMode?"Walk mode · Tap clear ground · Drag to orbit":!in3d?"Drag to explore · Tap a lot · Center lot to travel":
                meshRevision==serial&&meshSeed==seed&&meshX==cx&&meshY==cy?
                "Drag to orbit · Pinch to zoom · Tap a lot":"Loading 3D neighborhood…");
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
        if(inInterior)return;
        if(meshX==cx&&meshY==cy&&meshSeed==seed&&meshRevision==serial){
            world3d.select(selectedX,selectedY);
            hint.setText(walkMode?"Walk mode · Tap clear ground · Drag to orbit":"Drag to orbit · Pinch to zoom · Tap a lot");updateStreetResidents();return;
        }
        if(requestX==cx&&requestY==cy&&requestSeed==seed)return;
        final int version=++serial,x=cx,y=cy,key=seed;
        requestX=x;requestY=y;requestSeed=key;
        hint.setText("Loading 3D neighborhood…");
        meshWorker.execute(()->{
            if(version!=serial)return;
            float[] next;
            try{next=NativeWorld.mesh(x,y,4);}
            catch(OutOfMemoryError ex){next=null;}
            final float[] mesh=next;
            if(version!=serial)return;
            runOnUiThread(()->{
                if(version!=serial||isFinishing()||isDestroyed())return;
                Runnable failure=()->{
                    if(version!=serial||isFinishing()||isDestroyed())return;
                    requestX=requestY=meshX=meshY=meshRevision=-1;
                    in3d=false;showMode();hint.setText("3D scene unavailable — map remains usable");
                };
                if(mesh==null||mesh.length==0){failure.run();return;}
                world3d.setScene(mesh,x,y,version,()->{
                    if(version!=serial||isFinishing()||isDestroyed())return;
                    requestX=requestY=-1;meshX=x;meshY=y;meshSeed=key;meshRevision=version;
                    world3d.select(selectedX,selectedY);updateStreetResidents();
                    if(in3d&&!inInterior)hint.setText(walkMode?"Walk mode · Tap clear ground · Drag to orbit":"Drag to orbit · Pinch to zoom · Tap a lot");
                },failure);
                world3d.select(selectedX,selectedY);
            });
        });
    }
    private void advanceTime(int elapsed){
        elapsed=Math.max(0,elapsed);
        int total=minute+elapsed,days=total/1440;
        // Actions already account for their own energy and food costs.
        // Passive social/fun decay uses elapsed clock buckets so short trips
        // cannot reset the fractional portion of the timer.
        fun=clamp100(fun-LifeRules.elapsedBuckets(minute,elapsed,120));
        social=clamp100(social-LifeRules.elapsedBuckets(minute,elapsed,180));
        lifeDay=LifeRules.credit(lifeDay,days);weekday=(weekday+days)%7;minute=total%1440;
        int[] bill=household.rent(funds,rentDebt,days,rentedKey());funds=bill[0];rentDebt=bill[1];
    }
    private void notice(String message){Toast.makeText(this,message,Toast.LENGTH_SHORT).show();}
    private void visitSelected(){
        if(travelRoute!=null){cancelTravel(true);return;}
        if(!NativeWorld.canVisit(lifeX,lifeY)){notice("Walk off the bridge onto land before starting a longer trip");return;}
        if(!NativeWorld.canVisit(selectedX,selectedY)){notice("Choose a land lot to visit");return;}
        int[] route=NativeWorld.route(lifeX,lifeY,selectedX,selectedY);
        if(!LifeRules.validRoute(lifeX,lifeY,selectedX,selectedY,route)){notice("No local land route. Try a nearer lot or a bridge.");return;}
        int blocks=route.length;
        if(blocks==0){notice("Already at this lot");return;}
        int elapsed=LifeRules.travelMinutes(blocks),cost=LifeRules.travelEnergy(blocks);
        if(energy<cost){notice("Rest before traveling");return;}
        // Show the real native route. The trip commits once on arrival, never
        // saves the player halfway across a bridge, and cancels without cost.
        travelRoute=route;travelIndex=0;travelCost=cost;travelElapsed=elapsed;
        lastRoute=new int[blocks+1];lastRoute[0]=LifeRules.pack(lifeX,lifeY);
        System.arraycopy(route,0,lastRoute,1,blocks);
        travelButton.setText("Cancel trip");stepTravel();
    }
    private void stepTravel(){
        if(travelRoute==null)return;
        int n=travelRoute.length;
        travelIndex=Math.min(n,travelIndex+Math.max(1,(n+23)/24));
        int x=visualX(),y=visualY();
        world3d.player(x,y);map.invalidate();
        lifeStatus.setText("Traveling · "+travelIndex+"/"+n+" blocks\n"+travelElapsed+" game minutes · "+travelCost+" energy");
        if(travelIndex<n){travelHandler.postDelayed(travelTick,115);return;}
        lifeX=x;lifeY=y;resetOutdoorPosition();energy=clamp100(energy-travelCost);food=clamp100(food-travelElapsed/30);
        advanceTime(travelElapsed);trips=LifeRules.credit(trips,1);
        travelRoute=null;travelIndex=0;travelButton.setText("Travel here");
        cx=lifeX;cy=lifeY;selectedX=lifeX;selectedY=lifeY;refresh();saveLife();notice("Arrived");
    }
    private void cancelTravel(boolean announce){
        travelHandler.removeCallbacks(travelTick);
        if(travelRoute==null)return;
        travelRoute=null;travelIndex=0;lastRoute=new int[0];travelButton.setText("Travel here");
        updateDetails();if(announce)notice("Trip canceled");
    }
    private void buildWalkControls(){
        walkButton=new Button(this);walkButton.setAllCaps(false);walkButton.setText("Walk mode");walkButton.setTextSize(12);
        FrameLayout.LayoutParams toggle=new FrameLayout.LayoutParams(dp(122),dp(44),Gravity.TOP|Gravity.RIGHT);
        toggle.setMargins(0,dp(4),dp(4),0);pane.addView(walkButton,toggle);
        walkButton.setOnClickListener(v->{if(!inInterior&&travelRoute==null)toggleWalk();});
        walkControls=new LinearLayout(this);walkControls.setOrientation(LinearLayout.HORIZONTAL);
        FrameLayout.LayoutParams pad=new FrameLayout.LayoutParams(-1,dp(48),Gravity.BOTTOM);
        pane.addView(walkControls,pad);
        directionButton("North",0,-1);directionButton("West",-1,0);directionButton("East",1,0);directionButton("South",0,1);
        walkControls.setVisibility(View.GONE);
    }
    private void directionButton(String label,int dx,int dy){
        Button b=new Button(this);b.setText(label);b.setTextSize(12);b.setAllCaps(false);
        b.setMinWidth(0);b.setMinimumWidth(0);b.setPadding(0,0,0,0);
        walkControls.addView(b,new LinearLayout.LayoutParams(0,-1,1));
        b.setOnClickListener(v->{if(walkMode&&outdoorRoute==null)walkTo(tileX+dx,tileY+dy);});
    }
    private void resetOutdoorPosition(){tileX=lifeX*18+2;tileY=lifeY*18+2;}
    private OutdoorPath makeOutdoorMap(){
        int bx=Math.max(1,Math.min(1022,lifeX)),by=Math.max(1,Math.min(1022,lifeY));
        return OutdoorPath.create((bx-1)*18,(by-1)*18,NativeWorld.walkMask(bx,by));
    }
    private void toggleWalk(){
        if(walkMode){endWalkMode(true);return;}
        OutdoorPath p=makeOutdoorMap();
        int spawn=p==null?-1:p.nearest(OutdoorPath.pack(tileX,tileY));
        if(spawn<0||OutdoorPath.x(spawn)/18!=lifeX||OutdoorPath.y(spawn)/18!=lifeY){notice("No safe walking position on this lot");return;}
        tileX=OutdoorPath.x(spawn);tileY=OutdoorPath.y(spawn);outdoorMap=p;
        walkMode=true;in3d=true;cx=lifeX;cy=lifeY;selectedX=lifeX;selectedY=lifeY;
        walkButton.setText("Stop walking");world3d.followPlayer(true);showMode();refresh();saveLife();
    }
    private void walkTo(int tx,int ty){
        if(!walkMode||outdoorRoute!=null)return;
        OutdoorPath p=makeOutdoorMap();
        int[] route=p==null?null:p.route(OutdoorPath.pack(tileX,tileY),OutdoorPath.pack(tx,ty));
        if(route==null){notice("Choose reachable ground, not a building or water");return;}
        if(route.length==0)return;
        outdoorMap=p;outdoorRoute=route;walkIndex=walkDone=0;
        beginOutdoorStep();
    }
    private void beginOutdoorStep(){
        if(outdoorRoute==null)return;
        int next=outdoorRoute[walkIndex],at=OutdoorPath.pack(tileX,tileY);
        if(!outdoorMap.open(next)||!OutdoorPath.adjacent(at,next)){finishOutdoor();return;}
        world3d.playerTile(OutdoorPath.x(next),OutdoorPath.y(next),WALK_MS);
        travelHandler.postDelayed(outdoorTick,WALK_MS);
    }
    private void stepOutdoor(){
        if(outdoorRoute==null)return;
        int next=outdoorRoute[walkIndex++];tileX=OutdoorPath.x(next);tileY=OutdoorPath.y(next);walkDone++;
        if(walkIndex>=outdoorRoute.length)finishOutdoor();else beginOutdoorStep();
    }
    private void commitOutdoor(){
        travelHandler.removeCallbacks(outdoorTick);
        outdoorRoute=null;outdoorMap=null;
        if(walkDone>0)advanceTime((walkDone+5)/6);
        walkIndex=walkDone=0;
        lifeX=tileX/18;lifeY=tileY/18;selectedX=lifeX;selectedY=lifeY;
        world3d.playerTile(tileX,tileY,0);saveLife();
    }
    private void finishOutdoor(){
        commitOutdoor();cx=lifeX;cy=lifeY;refresh();saveLife();
    }
    private void endWalkMode(boolean refreshView){
        if(!walkMode)return;
        commitOutdoor();walkMode=false;walkButton.setText("Walk mode");world3d.followPlayer(false);
        if(refreshView){cx=lifeX;cy=lifeY;showMode();refresh();}
        else {showMode();updateDetails();}
        saveLife();
    }
    private int selectedLot(){
        int x=selectedX-cx+radius,y=selectedY-cy+radius,side=radius*2+1;
        if(x<0||y<0||x>=side||y>=side||tiles.length!=side*side)return -1;
        int code=tiles[y*side+x];return (code&7)==0?-1:(code>>>3)&63;
    }
    private void showFinder(){
        new AlertDialog.Builder(this).setTitle("Find nearby places")
            .setItems(new String[]{"Homes to rent","Workplaces","Food and groceries","Local venues","Tall buildings"},(d,i)->findPlaces(i))
            .setNegativeButton("Close",null).show();
    }
    private void findPlaces(int kind){
        int[] found=LifeRules.nearby(tiles,cx,cy,radius,lifeX,lifeY,kind,12);
        if(found.length==0){notice("No matching places in this map area. Explore or zoom out.");return;}
        String[] labels=new String[found.length];
        for(int i=0;i<found.length;i++){
            int x=LifeRules.x(found[i]),y=LifeRules.y(found[i]);
            labels[i]=NativeWorld.describe(x,y,weekday).split("\\n",2)[0]+(kind==LifeRules.TALL?" · "+NativeWorld.storeys(x,y)+" floors":"")+" · "+LifeRules.distance(lifeX,lifeY,x,y)+" blocks away";
        }
        new AlertDialog.Builder(this).setTitle(kind==LifeRules.HOMES?"Homes to rent":kind==LifeRules.WORK?"Workplaces":kind==LifeRules.FOOD?"Food nearby":kind==LifeRules.TALL?"Tall buildings":"Local venues")
            .setItems(labels,(d,i)->{selectedX=LifeRules.x(found[i]);selectedY=LifeRules.y(found[i]);updateDetails();notice("Selected — tap Travel here");})
            .setNegativeButton("Close",null).show();
    }
    private void homeAction(){
        if(atHome()){enterHome(-1);return;}
        if(atSelected()&&LifeRules.isHome(selectedLot())){
            if(rentDebt>0){notice("Clear your overdue rent before moving");return;}
            if(funds<LifeRules.LEASE){notice("A lease costs $"+LifeRules.LEASE+". Find work to earn it.");return;}
            final int x=lifeX,y=lifeY;
            new AlertDialog.Builder(this).setTitle("Lease this home?")
                .setMessage("$"+LifeRules.LEASE+" to move in, then $"+LifeRules.DAILY_RENT+" per game day. Sleep, cook and spend time at home.")
                .setPositiveButton("Move in",(d,i)->{
                    if(lifeX!=x||lifeY!=y||funds<LifeRules.LEASE||rentDebt>0)return;
                    funds-=LifeRules.LEASE;homeX=x;homeY=y;savedDecor="";homeDecor=null;household.clear();updateDetails();saveLife();notice("Welcome home");
                }).setNegativeButton("Cancel",null).show();return;
        }
        if(!hasHome()){findPlaces(LifeRules.HOMES);return;}
        selectedX=homeX;selectedY=homeY;
        if(Math.abs(selectedX-cx)>radius||Math.abs(selectedY-cy)>radius){cx=homeX;cy=homeY;refresh();}
        else updateDetails();
        notice("Home selected — tap Travel here");
    }
    private void showCharacterEditor(){
        final AvatarStyle[] draft={appearance};
        ScrollView scroll=new ScrollView(this);LinearLayout content=new LinearLayout(this);content.setOrientation(LinearLayout.VERTICAL);content.setPadding(dp(16),dp(8),dp(16),dp(8));scroll.addView(content);
        AvatarPreview preview=new AvatarPreview(this,draft[0]);content.addView(preview,new LinearLayout.LayoutParams(-1,dp(136)));
        EditText name=new EditText(this);name.setSingleLine(true);name.setHint("Character name");name.setContentDescription("Character name");name.setText(playerName);name.setFilters(new android.text.InputFilter[]{new android.text.InputFilter.LengthFilter(40)});content.addView(name,new LinearLayout.LayoutParams(-1,-2));
        for(int k=0;k<AvatarStyle.FIELDS.length;k++){
            final int field=k;LinearLayout row=new LinearLayout(this);row.setGravity(Gravity.CENTER_VERTICAL);content.addView(row,new LinearLayout.LayoutParams(-1,-2));
            TextView label=new TextView(this);label.setText(AvatarStyle.FIELDS[k]);label.setTextSize(13);row.addView(label,new LinearLayout.LayoutParams(0,-2,1));
            Spinner choice=new Spinner(this);choice.setContentDescription(AvatarStyle.FIELDS[k]);
            ArrayAdapter<String> adapter=new ArrayAdapter<>(this,android.R.layout.simple_spinner_item,AvatarStyle.options(k));adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);choice.setAdapter(adapter);choice.setSelection(draft[0].choice(k));
            row.addView(choice,new LinearLayout.LayoutParams(0,dp(42),1));
            choice.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener(){
                @Override public void onItemSelected(AdapterView<?> parent,View view,int position,long id){draft[0]=draft[0].with(field,position);preview.appearance(draft[0]);}
                @Override public void onNothingSelected(AdapterView<?> parent){}
            });
        }
        AlertDialog editor=new AlertDialog.Builder(this).setTitle("Customize character").setView(scroll)
            .setPositiveButton("Save appearance",(dialog,which)->{
                appearance=draft[0];playerName=AvatarStyle.cleanName(name.getText().toString());world3d.appearance(appearance);interiorView.appearance(appearance);updateDetails();saveLife();notice("Appearance saved");
            }).setNegativeButton("Cancel",null).create();
        editor.getWindow().setSoftInputMode(android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);editor.show();
    }
    private void buildHomePane(){
        homePane=new LinearLayout(this);homePane.setOrientation(LinearLayout.VERTICAL);homePane.setBackgroundColor(BG);
        roomCaption=new TextView(this);roomCaption.setTextColor(INK);roomCaption.setTextSize(13);
        roomCaption.setGravity(Gravity.CENTER);roomCaption.setPadding(dp(8),dp(5),dp(8),dp(4));homePane.addView(roomCaption);
        interiorView=new InteriorView(this,new InteriorView.Listener(){
            @Override public void arrived(int action,int cell){interiorArrived(action,cell);}
            @Override public void message(String message){notice(message);}
            @Override public void moveFurniture(int slot,int cell){relocateFurniture(slot,cell);}
        });
        interiorView.appearance(appearance);homePane.addView(interiorView,new LinearLayout.LayoutParams(-1,0,1));
        LinearLayout row=new LinearLayout(this);homePane.addView(row);
        LinearLayout row2=new LinearLayout(this);homePane.addView(row2);
        for(int k=0;k<InteriorPlan.COUNT;k++){
            final int slot=k;roomButtons[k]=homeButton(k<3?row:row2,"",()->interiorView.use(slot));
        }
        exitRoomButton=homeButton(row2,"Exit home",interiorView::exit);
        stairsRow=new LinearLayout(this);homePane.addView(stairsRow);
        downstairsButton=homeButton(stairsRow,"Downstairs",()->changeFloor(-1));
        upstairsButton=homeButton(stairsRow,"Upstairs",()->changeFloor(1));
        stairsRow.setVisibility(View.GONE);
        decorRow=new LinearLayout(this);homePane.addView(decorRow);
        decorateButton=homeButton(decorRow,"Decorate home",()->setDecorating(!decorating));
        householdButton=homeButton(decorRow,"Household",this::showHousehold);
        nextSpotButton=homeButton(decorRow,"Next spot",this::nextFurnitureSpot);
        themeButton=homeButton(decorRow,"Room style",this::chooseRoomStyle);
        resetRoomButton=homeButton(decorRow,"Reset room",this::confirmRoomReset);
        decorRow.setVisibility(View.GONE);
        pane.addView(homePane,new FrameLayout.LayoutParams(-1,-1));homePane.setVisibility(View.GONE);
    }
    private Button homeButton(LinearLayout row,String title,Runnable action){
        Button b=new Button(this);b.setAllCaps(false);b.setText(title);b.setTextSize(12);
        b.setMinWidth(0);b.setMinimumWidth(0);b.setPadding(0,0,0,0);
        row.addView(b,new LinearLayout.LayoutParams(0,dp(44),1));b.setOnClickListener(v->{if(inInterior)action.run();});return b;
    }
    private void enterHome(int saved){enterHome(saved,0);}
    private void enterHome(int saved,int floor){
        if(!atHome()||travelRoute!=null)return;
        BuildingPlan p=BuildingPlan.create(NativeWorld.interior(homeX,homeY),seed^LifeRules.pack(homeX,homeY),NativeWorld.storeys(homeX,homeY),false);
        if(p==null){notice("This home has no accessible interior yet");return;}
        openInterior(p,saved,floor);
    }
    private void enterSelected(){
        if(!atSelected()){notice("Travel to the selected building first");return;}
        if(atHome()){enterHome(-1);return;}
        if(!enterVenue(-1,-1,-1))notice("No public interior at this location");
    }
    private boolean enterVenue(int saved,int service,int expectedLot){return enterVenue(saved,service,expectedLot,0);}
    private boolean enterVenue(int saved,int service,int expectedLot,int floor){
        if(inInterior||travelRoute!=null||!atSelected()||atHome())return false;
        BuildingPlan p=BuildingPlan.create(NativeWorld.placeInterior(lifeX,lifeY),seed^LifeRules.pack(lifeX,lifeY),NativeWorld.storeys(lifeX,lifeY),true);
        if(p==null||(expectedLot>=0&&p.floor(0).lot!=expectedLot))return false;
        int serviceFloor=p.floorFor(service);if(service>=0&&serviceFloor<0)return false;
        openInterior(p,saved,floor);
        if(service>=0){
            int slot=interiorPlan.slotFor(service);
            if(slot>=0)interiorView.use(slot);else notice("Use the stairs to floor "+(serviceFloor+1)+" for "+VenueRules.label(interiorPlan.lot,service));
        }
        return true;
    }
    private boolean requestVenueService(int service){
        if(inInterior||!atSelected()||!VenueRules.has(selectedLot(),service))return false;
        if(!enterVenue(-1,service,-1))notice("This venue has no accessible station yet");
        return true;
    }
    private boolean interiorCurrent(){
        return inInterior&&interiorPlan!=null&&interiorX==lifeX&&interiorY==lifeY&&atSelected()&&(interiorPlan.commercial||atHome());
    }
    private boolean atService(int service){
        if(!interiorCurrent())return false;
        int slot=interiorPlan.slotFor(service);return !interiorView.busy()&&slot>=0&&interiorPlan.canUse(interiorView.cell(),slot);
    }
    private void openInterior(BuildingPlan p,int saved,int floor){
        selectedX=lifeX;selectedY=lifeY;interiorX=lifeX;interiorY=lifeY;buildingPlan=p;inInterior=true;
        homeDecor=p.floor(0).commercial?null:HomeDecor.create(p,seed,rentedKey(),savedDecor);
        if(homeDecor!=null)savedDecor=homeDecor.save();
        showFloor(p.restoreFloor(floor),floor==p.restoreFloor(floor)?saved:-1);
        showMode();updateDetails();saveLife();
    }
    private void showFloor(int floor,int saved){
        roomFloor=floor;InteriorPlan p=buildingPlan.floor(floor);interiorPlan=p;
        String name=p.commercial?NativeWorld.describe(lifeX,lifeY,weekday).split("\\n",2)[0]:"YOUR HOME";
        roomCaption.setText(name+" · Floor "+(floor+1)+"/"+buildingPlan.count());
        for(int k=0;k<roomButtons.length;k++){roomButtons[k].setText(p.labelAt(k));roomButtons[k].setVisibility(k<p.actionCount()?View.VISIBLE:View.GONE);}
        exitRoomButton.setText(p.commercial?"Exit building":"Exit home");exitRoomButton.setVisibility(floor==0?View.VISIBLE:View.GONE);
        stairsRow.setVisibility(buildingPlan.count()>1?View.VISIBLE:View.GONE);
        downstairsButton.setEnabled(floor>0);upstairsButton.setEnabled(floor+1<buildingPlan.count());
        interiorView.open(p,saved,buildingPlan.landing);
        decorating=false;interiorView.setTheme(homeDecor==null?0:homeDecor.theme(floor));refreshDecorControls();
    }
    private void refreshDecorControls(){
        if(interiorPlan==null)return;
        boolean home=homeDecor!=null&&!interiorPlan.commercial;
        decorRow.setVisibility(home?View.VISIBLE:View.GONE);
        decorateButton.setText(decorating?"Done decorating":"Decorate home");
        householdButton.setVisibility(home&&!decorating?View.VISIBLE:View.GONE);
        nextSpotButton.setVisibility(decorating?View.VISIBLE:View.GONE);
        themeButton.setVisibility(decorating?View.VISIBLE:View.GONE);
        resetRoomButton.setVisibility(decorating?View.VISIBLE:View.GONE);
        exitRoomButton.setVisibility(!decorating&&roomFloor==0?View.VISIBLE:View.GONE);
        stairsRow.setVisibility(!decorating&&buildingPlan.count()>1?View.VISIBLE:View.GONE);
        interiorView.setDecorating(decorating);
        hint.setText(decorating?"Select furniture, then tap an open tile or Next spot":interiorPlan.commercial?"Venue · walk to a service station":"Home · tap a floor tile or use furniture");
    }
    private void setDecorating(boolean enabled){
        if(!interiorCurrent()||homeDecor==null)return;
        if(enabled&&interiorView.busy()){notice("Finish walking before decorating");return;}
        decorating=enabled;refreshDecorControls();saveLife();
    }
    private boolean validRoommateResidence(int key){
        if(!Household.local(key,rentedKey())||NativeWorld.storeys(homeX,homeY)<2)return false;
        ResidentSchedule.Grid grid=residentGridAt(homeX,homeY,6);
        if(grid==null||!LifeRules.isHome(grid.lot(homeX,homeY))||!LifeRules.isHome(grid.lot(LifeRules.x(key),LifeRules.y(key))))return false;
        return LifeRules.validRoute(homeX,homeY,LifeRules.x(key),LifeRules.y(key),NativeWorld.route(homeX,homeY,LifeRules.x(key),LifeRules.y(key)));
    }
    private ResidentSchedule.Entry roommateLocation(){
        int key=household.member();if(key<0||!hasHome())return null;
        ResidentSchedule.Grid grid=residentGridAt(homeX,homeY,6);
        return grid==null?null:grid.locatedAt(seed,key,rentedKey(),weekday,minute);
    }
    private void showHousehold(){
        if(!interiorCurrent()||!atHome()||decorating||interiorView.busy()){notice("Finish your current action first");return;}
        if(household.member()<0){
            String reason=buildingPlan.count()<2?"A shared home needs a second floor with another bed.":rentDebt>0?"Settle overdue rent before inviting someone.":"Invite a nearby friend with a relationship of at least 30. A roommate contributes $4 toward the daily $12 rent.";
            new AlertDialog.Builder(this).setTitle("Household").setMessage("You live alone.\nDaily rent: $12\n\n"+reason)
                .setPositiveButton(buildingPlan.count()>=2&&rentDebt==0?"Invite roommate":"Close",(d,i)->{if(buildingPlan.count()>=2&&rentDebt==0)chooseRoommate();})
                .setNegativeButton("Cancel",null).show();return;
        }
        final int key=household.member();
        MobileContacts.Resident r=resident(key);
        ResidentSchedule.Entry e=roommateLocation();
        String[] phases={"At home","Working","Lunch","Leisure","Sleeping","Traveling"};
        String status=e==null?"Away":phases[e.phase];
        if(e!=null&&e.location!=rentedKey())status+=" · Block "+LifeRules.x(e.location)+", "+LifeRules.y(e.location);
        boolean together=e!=null&&e.phase==ResidentSchedule.HOME&&e.location==rentedKey();
        new AlertDialog.Builder(this).setTitle("Household")
            .setMessage(r.name+" · "+MobileContacts.TRAITS[r.trait]+"\n"+status+"\nRelationship "+contacts.affinity(key)+"/100\n\nRent: $8/day after roommate contribution.")
            .setPositiveButton(together?"Chat together":"Close",(d,i)->{if(together&&household.member()==key)showConversation(r);})
            .setNeutralButton("Move out",(d,i)->confirmRoommateDeparture(key)).setNegativeButton("Cancel",null).show();
    }
    private void chooseRoommate(){
        if(!interiorCurrent()||!atHome()||household.member()>=0||buildingPlan.count()<2||rentDebt!=0)return;
        java.util.ArrayList<Integer> candidates=new java.util.ArrayList<>();
        java.util.ArrayList<String> labels=new java.util.ArrayList<>();
        for(int key:contacts.keys())if(Household.local(key,rentedKey())&&contacts.affinity(key)>=Household.REQUIRED_AFFINITY&&validRoommateResidence(key)){
            candidates.add(key);labels.add(resident(key).name+" · "+contacts.affinity(key)+"/100");
        }
        if(candidates.isEmpty()){notice("Build a relationship of 30 with a nearby resident first");return;}
        new AlertDialog.Builder(this).setTitle("Invite roommate").setItems(labels.toArray(new String[0]),(d,i)->{
            int key=candidates.get(i);if(!interiorCurrent()||!atHome()||!validRoommateResidence(key))return;
            ResidentSchedule.Grid grid=residentGridAt(homeX,homeY,6);
            ResidentSchedule.Entry e=grid==null?null:grid.located(seed,key,weekday,minute);
            boolean free=e!=null&&(e.phase==ResidentSchedule.HOME||e.phase==ResidentSchedule.LEISURE);
            if(household.invite(key,rentedKey(),contacts.affinity(key),rentDebt,true,free)){
                residentRevision=-1;updateDetails();saveLife();notice(resident(key).name+" accepted and moved in");
            }else notice("They are unavailable now, or the household is full");
        }).setNegativeButton("Cancel",null).show();
    }
    private void confirmRoommateDeparture(int key){
        new AlertDialog.Builder(this).setTitle("End shared household?").setMessage(resident(key).name+" will return to their original home. Your daily rent returns to $12.")
            .setPositiveButton("Confirm move out",(d,i)->{
                if(!interiorCurrent()||!atHome()||household.member()!=key)return;
                household.clear();residentRevision=-1;updateDetails();saveLife();notice("Household updated");
            }).setNegativeButton("Cancel",null).show();
    }
    private void saveDecor(){
        if(homeDecor==null)return;
        savedDecor=homeDecor.save();interiorView.setTheme(homeDecor.theme(roomFloor));interiorView.invalidate();saveLife();
    }
    private void relocateFurniture(int slot,int target){
        if(!decorating||!interiorCurrent()||homeDecor==null||interiorView.busy())return;
        if(homeDecor.move(roomFloor,slot,target,interiorView.cell())){saveDecor();notice("Furniture moved");}
        else notice("Keep the doorway, stairs, player and furniture routes clear");
    }
    private void nextFurnitureSpot(){
        if(!decorating||!interiorCurrent()||homeDecor==null)return;
        int slot=interiorView.selectedFurniture();
        if(slot<0){notice("Select furniture first");return;}
        if(homeDecor.nextSpot(roomFloor,slot,interiorView.cell())>=0){saveDecor();notice("Furniture moved");}
        else notice("No alternative clear position");
    }
    private void chooseRoomStyle(){
        if(!decorating||!interiorCurrent()||homeDecor==null)return;
        final HomeDecor target=homeDecor;final int floor=roomFloor;
        new AlertDialog.Builder(this).setTitle("Room style").setSingleChoiceItems(HomeDecor.THEMES,target.theme(floor),(dialog,which)->{
            if(decorating&&homeDecor==target&&roomFloor==floor&&target.setTheme(floor,which))saveDecor();
            dialog.dismiss();
        }).setNegativeButton("Cancel",null).show();
    }
    private void confirmRoomReset(){
        if(!decorating||!interiorCurrent()||homeDecor==null)return;
        final HomeDecor target=homeDecor;final int floor=roomFloor;
        new AlertDialog.Builder(this).setTitle("Reset this floor?").setMessage("Restore the original furniture positions and room colors on this floor only.")
            .setPositiveButton("Reset",(dialog,which)->{
                if(!decorating||homeDecor!=target||roomFloor!=floor)return;
                if(target.reset(floor,interiorView.cell())){saveDecor();notice("Room restored");}
                else notice("Move your character clear of the original furniture first");
            }).setNegativeButton("Cancel",null).show();
    }
    private void changeFloor(int direction){
        if(decorating||!interiorCurrent()||buildingPlan==null||buildingPlan.floor(roomFloor+direction)==null)return;
        interiorView.climb(direction);
    }
    private void leaveHome(){
        if(!inInterior||roomFloor!=0)return;
        inInterior=false;decorating=false;interiorView.close();interiorPlan=null;buildingPlan=null;homeDecor=null;roomFloor=-1;interiorX=interiorY=-1;
        cx=lifeX;cy=lifeY;selectedX=lifeX;selectedY=lifeY;
        showMode();refresh();saveLife();
    }
    private void interiorArrived(int action,int cell){
        if(decorating||!interiorCurrent())return;
        if(action==InteriorView.UP||action==InteriorView.DOWN){
            int direction=action==InteriorView.UP?1:-1;
            if(buildingPlan!=null&&buildingPlan.canChange(roomFloor,cell,direction)){
                showFloor(roomFloor+direction,buildingPlan.landing);advanceTime(2);updateDetails();saveLife();
            }
            return;
        }
        if(action==InteriorView.EXIT){if(roomFloor==0&&cell==interiorPlan.entry)leaveHome();return;}
        if(action==InteriorView.WALK){saveLife();return;}
        if(!interiorPlan.canUse(cell,action))return;
        switch(interiorPlan.actionAt(action)){
            case VenueRules.WORK:workShift();break;
            case VenueRules.MEAL:eatMeal();break;
            case VenueRules.ACTIVITY:showActivities();break;
            case VenueRules.MEET:showLocals();break;
            case VenueRules.WAIT:rest();break;
            case InteriorPlan.BED:sleepAtHome();break;
            case InteriorPlan.KITCHEN:eatMeal();break;
            case InteriorPlan.HOBBY:
                if(energy<8){notice("Rest before practicing");break;}
                energy-=8;food=clamp100(food-3);fun=clamp100(fun+25);skills[3]=Math.min(4400,skills[3]+20);
                advanceTime(60);updateDetails();saveLife();notice("Creativity improved");break;
            case BuildingPlan.STUDY:
                if(!atHome()||energy<6){notice("Rest before studying");break;}
                energy-=6;food=clamp100(food-3);skills[1]=Math.min(4400,skills[1]+22);advanceTime(45);updateDetails();notice("Logic improved");break;
            case BuildingPlan.FITNESS:
                if(!atHome()||energy<12||food<8){notice("Eat and rest before exercising");break;}
                energy-=12;food=clamp100(food-8);fun=clamp100(fun+8);skills[2]=Math.min(4400,skills[2]+24);advanceTime(45);updateDetails();notice("Fitness improved");break;
            case InteriorPlan.PHONE:showPhone();break;
            case InteriorPlan.SOFA:rest();break;
            default:break;
        }
        saveLife();
    }
    @Override public void onBackPressed(){
        if(walkMode){endWalkMode(true);return;}
        if(inInterior){if(decorating){setDecorating(false);return;}if(interiorView.busy()){interiorView.cancel();saveLife();}else if(roomFloor>0)changeFloor(-1);else interiorView.exit();return;}
        super.onBackPressed();
    }
    private MobileContacts.Resident resident(int key){return new MobileContacts.Resident(seed,key);}
    private ResidentSchedule.Grid residentGridAt(int x,int y,int margin){
        if(residentGrid!=null&&residentGridSeed==seed&&residentGrid.covers(x,y,margin))return residentGrid;
        final int r=10,X=Math.max(r,Math.min(1023-r,x)),Y=Math.max(r,Math.min(1023-r,y));
        residentGrid=ResidentSchedule.Grid.create(X,Y,r,NativeWorld.tiles(X,Y,r));residentGridSeed=seed;
        return residentGrid;
    }
    private void updateStreetResidents(){
        if(!in3d||inInterior||meshRevision!=serial||meshSeed!=seed||meshX!=cx||meshY!=cy)return;
        int bucket=minute/6,home=rentedKey(),roommate=household.member();
        if(residentRevision==meshRevision&&residentDay==weekday&&residentTime==bucket&&residentHome==home&&residentRoommate==roommate)return;
        ResidentSchedule.Grid grid=residentGridAt(cx,cy,8);
        if(grid==null)return;
        int[] people=grid.streetActors(seed,cx,cy,2,weekday,minute,home,roommate,16);
        world3d.residents(people,meshRevision);
        residentRevision=meshRevision;residentDay=weekday;residentTime=bucket;residentHome=home;residentRoommate=roommate;
    }
    private java.util.ArrayList<MobileContacts.Resident> localResidents(){
        java.util.ArrayList<MobileContacts.Resident> people=new java.util.ArrayList<>();
        int lot=selectedLot();
        if(!atSelected()||!(MobileContacts.isSocialLot(lot)||LifeRules.isWork(lot)))return people;
        ResidentSchedule.Grid grid=residentGridAt(lifeX,lifeY,6);if(grid==null)return people;
        for(ResidentSchedule.Entry e:grid.snapshot(seed,lifeX,lifeY,0,weekday,minute,rentedKey(),household.member())){
            if(e.phase!=ResidentSchedule.SLEEP&&e.phase!=ResidentSchedule.COMMUTE&&people.size()<8)people.add(resident(e.home));
        }
        return people;
    }
    private void showLocals(){
        if(!atSelected()){notice("Travel to this lot to meet people");return;}
        if(atHome()){showPhone();return;}
        if(requestVenueService(VenueRules.MEET))return;
        if(inInterior&&!atService(VenueRules.MEET))return;
        java.util.ArrayList<MobileContacts.Resident> people=localResidents();
        if(people.isEmpty()){notice("Nobody is here right now. Try a nearby workplace during the day, food venue at lunch or park in the evening.");return;}
        String[] labels=new String[people.size()];
        for(int i=0;i<labels.length;i++){MobileContacts.Resident r=people.get(i);labels[i]=r.name+" · "+MobileContacts.TRAITS[r.trait]+" · relationship "+contacts.affinity(r.key);}
        new AlertDialog.Builder(this).setTitle("People here").setItems(labels,(d,i)->showConversation(people.get(i)))
            .setNegativeButton("Close",null).show();
    }
    private void showConversation(MobileContacts.Resident r){
        new AlertDialog.Builder(this).setTitle(r.name+" · "+MobileContacts.TRAITS[r.trait])
            .setItems(MobileContacts.TOPICS,(d,i)->{
                boolean present=false;for(MobileContacts.Resident p:localResidents())if(p.key==r.key)present=true;
                if(!present){notice("They have left");return;}talkTo(r,i,false);
            }).setNegativeButton("Close",null).show();
    }
    private void showContactList(){
        int[] keys=contacts.keys();if(keys.length==0){notice("Meet someone in the neighborhood to add a contact");return;}
        String[] labels=new String[keys.length];
        for(int i=0;i<keys.length;i++){MobileContacts.Resident r=resident(keys[i]);labels[i]=r.name+" · "+MobileContacts.TRAITS[r.trait]+" · "+contacts.affinity(r.key)+"/100";}
        new AlertDialog.Builder(this).setTitle("Your contacts").setItems(labels,(d,i)->{
            if(atHome())talkTo(resident(keys[i]),0,true);else notice("Phone contacts from home, or meet them locally");
        }).setNegativeButton("Close",null).show();
    }
    private void showPhone(){
        if(!atHome()){notice("Return home to call a contact");return;}
        showContactList();
    }
    private void talkTo(MobileContacts.Resident r,int topic,boolean phone){
        if(phone&&!atHome())return;
        if(energy<2){notice("Rest before talking");return;}
        int gain=contacts.talk(r,topic,lifeDay,minute,phone);
        if(gain<=0){notice(gain==-1?"Give them some time before chatting again":gain==-2?"Meet them in person first":"Contact book is full");return;}
        social=clamp100(social+8+gain);fun=clamp100(fun+(gain>=8?5:1));energy-=2;
        skills[6]=Math.min(4400,skills[6]+gain);advanceTime(30);updateDetails();saveLife();
        notice(r.name+(gain>=8?" enjoyed that topic":" appreciated the chat"));
    }
    private void sleepAtHome(){
        if(!atHome()){notice(hasHome()?"Travel home before sleeping":"Rent a home first");return;}
        advanceTime(480);energy=100;food=clamp100(food-12);homeNights=LifeRules.credit(homeNights,1);
        updateDetails();saveLife();notice("Well rested");
    }
    private void rest(){
        advanceTime(60);energy=clamp100(energy+(atHome()?20:12));food=clamp100(food-3);
        updateDetails();saveLife();notice("Rested for one hour");
    }
    private void showGoals(){
        String message=(hasHome()?"✓":"○")+" Lease a home\n"+(homeNights>0?"✓":"○")+" Sleep at home\n"+
            (trips>=3?"✓":"○")+" Explore three locations ("+Math.min(trips,3)+"/3)\n"+
            (careerShifts>=4?"✓":"○")+" Complete four shifts ("+Math.min(careerShifts,4)+"/4)\n"+
            (sessions>=3?"✓":"○")+" Join three local activities ("+Math.min(sessions,3)+"/3)\n\n"+
            "Career bonus: $"+LifeRules.payBonus(careerShifts)+" per shift\nRent overdue: $"+rentDebt+"\nContacts: "+contacts.keys().length;
        new AlertDialog.Builder(this).setTitle("Your life goals").setMessage(message).setPositiveButton("Keep playing",null).show();
    }
    private void workShift(){
        if(lifeX!=selectedX||lifeY!=selectedY){notice("Travel to this workplace first");return;}
        if(requestVenueService(VenueRules.WORK))return;
        if(inInterior&&!atService(VenueRules.WORK))return;
        if(lastShiftDay==lifeDay){notice("You have already worked a shift today");return;}
        int[] v=NativeWorld.shift(lifeX,lifeY,weekday,minute,energy,food);
        if(v==null){notice("No work shifts at this location");return;}
        if(v[0]!=0){notice(v[0]==1?"No shift fits the opening hours":v[0]==2?"Rest before working":"Eat before working");return;}
        lastShiftDay=lifeDay;int pay=v[2]+LifeRules.payBonus(careerShifts);
        funds=LifeRules.credit(funds,pay);careerShifts=LifeRules.credit(careerShifts,1);
        energy=clamp100(energy-v[3]);food=clamp100(food-v[4]);
        if(v[5]>=0&&v[5]<skills.length)skills[v[5]]=Math.min(4400,skills[v[5]]+v[6]);
        advanceTime(v[1]);updateDetails();saveLife();notice("Shift complete: earned $"+pay);
    }
    private void eatMeal(){
        boolean cooking=atHome();
        if(cooking&&inInterior&&!atService(InteriorPlan.KITCHEN))return;
        if(!cooking&&(!atSelected()||!LifeRules.isFood(selectedLot()))){notice("Eat at home or travel to a food venue");return;}
        if(!cooking&&requestVenueService(VenueRules.MEAL))return;
        if(!cooking&&inInterior&&!atService(VenueRules.MEAL))return;
        if(!cooking&&VenueRules.supported(selectedLot())&&!VenueRules.mealOpen(selectedLot(),minute)){notice("Food service is closed");return;}
        int price=cooking?5:8;
        if(funds<price){notice("Not enough money for a meal");return;}
        funds-=price;food=clamp100(food+(cooking?45:35));
        if(cooking)skills[4]=Math.min(4400,skills[4]+12);
        advanceTime(30);updateDetails();saveLife();notice(cooking?"Cooked a meal":"Ate a meal");
    }
    private void showActivities(){
        if(lifeX!=selectedX||lifeY!=selectedY){notice("Travel to this lot first");return;}
        if(requestVenueService(VenueRules.ACTIVITY))return;
        if(inInterior&&!atService(VenueRules.ACTIVITY))return;
        int[] a=NativeWorld.activity(lifeX,lifeY,weekday,0,minute,funds,energy);
        int[] b=NativeWorld.activity(lifeX,lifeY,weekday,1,minute,funds,energy);
        if(a==null||b==null){notice("No scheduled venue activities on this lot");return;}
        int[][] options={a,b};
        String[] labels=new String[2];
        for(int i=0;i<2;i++){
            int[] v=options[i];labels[i]=NativeWorld.activityText(v[1],0)+" ($"+v[2]+", "+v[10]+" min)\n"+NativeWorld.activityText(v[0],2);
        }
        new AlertDialog.Builder(this).setTitle("Local activities").setItems(labels,(d,i)->finishActivity(i,options[i][1])).setNegativeButton("Close",null).show();
    }
    private void finishActivity(int slot,int expected){
        if(lifeX!=selectedX||lifeY!=selectedY)return;
        if(VenueRules.supported(selectedLot())&&!atService(VenueRules.ACTIVITY))return;
        int[] v=NativeWorld.activity(lifeX,lifeY,weekday,slot,minute,funds,energy);
        if(v==null||v[1]!=expected){notice("The venue schedule changed");return;}
        if(v[0]!=0){notice(NativeWorld.activityText(v[0],2));return;}
        funds-=v[2];energy=clamp100(energy-v[3]+v[7]);fun=clamp100(fun+v[4]);
        social=clamp100(social+v[5]);food=clamp100(food+v[6]);
        if(v[8]>=0&&v[8]<skills.length)skills[v[8]]=Math.min(4400,skills[v[8]]+v[9]);
        advanceTime(v[10]);sessions=LifeRules.credit(sessions,1);updateDetails();saveLife();
        notice(NativeWorld.activityText(v[1],1));
    }
    private void saveLife(){
        SharedPreferences.Editor e=prefs.edit().putInt("seed",seed).putInt("lifeX",lifeX).putInt("lifeY",lifeY)
             .putInt("day",weekday).putInt("minute",minute).putInt("funds",funds).putInt("energy",energy)
             .putInt("fun",fun).putInt("social",social).putInt("food",food).putInt("sessions",sessions)
             .putInt("lifeDay",lifeDay).putInt("lastShiftDay",lastShiftDay)
             .putInt("homeX",homeX).putInt("homeY",homeY).putInt("rentDebt",rentDebt)
             .putInt("careerShifts",careerShifts).putInt("trips",trips).putInt("homeNights",homeNights)
             .putInt("x",cx).putInt("y",cy).putString("contacts",contacts.save())
             .putBoolean("insideHome",inInterior&&!interiorPlan.commercial).putBoolean("insideVenue",inInterior&&interiorPlan.commercial)
             .putInt("insideX",interiorX).putInt("insideY",interiorY).putInt("insideLot",inInterior?interiorPlan.lot:-1)
             .putInt("roomCell",inInterior?interiorView.cell():-1).putInt("roomFloor",roomFloor)
             .putInt("tileX",tileX).putInt("tileY",tileY).putBoolean("crowdMotion",crowdMotion)
             .putInt("avatarStyle",appearance.encode()).putString("avatarName",playerName).putString("homeDecor",savedDecor).putString("household",household.save(seed)).putInt("roommate",household.member());
        for(int i=0;i<skills.length;i++)e.putInt("skill"+i,skills[i]);e.apply();
    }
    private void updateDetails(){
        String text=NativeWorld.describe(selectedX,selectedY,weekday);
        detail.setText(text+"\nBlock "+selectedX+", "+selectedY+"  ·  Seed "+Integer.toUnsignedString(seed));
        int best=0;for(int i=1;i<skills.length;i++)if(skills[i]>skills[best])best=i;
        String day=new String[]{"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}[weekday];
        lifeStatus.setText(String.format(java.util.Locale.US,"%s %02d:%02d · $%d · Energy %d\nFood %d · Fun %d · Social %d\n%s · %s %d\n%s · Shifts %d · Trips %d",
            day,minute/60,minute%60,funds,energy,food,fun,social,
            atSelected()?playerName+" · Here":playerName+": "+lifeX+", "+lifeY,SKILL_NAMES[best],skills[best],
            !hasHome()?"Find a home":(atHome()?"At home":"Home "+homeX+", "+homeY)+(rentDebt>0?" · Rent due $"+rentDebt:""),careerShifts,trips));
        if(household.member()>=0)lifeStatus.append("\nRoommate: "+resident(household.member()).name+" · Rent $"+household.dailyRent(rentedKey())+"/day");
        map.setContentDescription("San Boreas map. Selected lot: "+text);
        world3d.setContentDescription("San Boreas 3D neighborhood. Selected lot: "+text);
        world3d.setClock(minute);world3d.playerTile(tileX,tileY,0);world3d.select(selectedX,selectedY);updateStreetResidents();map.invalidate();
    }
    @Override protected void onResume(){
        super.onResume();resumed=true;
        if(world3d!=null){world3d.onResume();world3d.setCrowdActive(in3d&&!inInterior&&crowdMotion);if(in3d){requestScene();world3d.requestRender();}}
    }
    @Override protected void onPause(){
        resumed=false;if(world3d!=null)world3d.setCrowdActive(false);
        cancelTravel(false);if(walkMode)endWalkMode(false);if(inInterior)interiorView.cancel();saveLife();
        prefs.edit().putInt("seed",seed).putInt("x",cx).putInt("y",cy)
             .putInt("radius",radius).putBoolean("weekend",weekday==5).putBoolean("view3d",in3d).apply();
        if(world3d!=null)world3d.onPause();super.onPause();
    }
    @Override protected void onDestroy(){if(world3d!=null)world3d.setCrowdActive(false);if(interiorView!=null)interiorView.close();travelHandler.removeCallbacks(travelTick);travelHandler.removeCallbacks(outdoorTick);serial++;meshWorker.shutdownNow();super.onDestroy();}
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
            paint.setAntiAlias(true);paint.setColor(0xff80d9cd);paint.setStrokeWidth(Math.max(2,step*.12f));
            for(int i=1;i<lastRoute.length;i++){
                int a=lastRoute[i-1],b=lastRoute[i];
                c.drawLine(lx+((a&65535)-cx+radius+.5f)*step+offsetX,ly+((a>>>16)-cy+radius+.5f)*step+offsetY,
                    lx+((b&65535)-cx+radius+.5f)*step+offsetX,ly+((b>>>16)-cy+radius+.5f)*step+offsetY,paint);
            }
            if(hasHome()){
                int hx=homeX-cx+radius,hy=homeY-cy+radius;
                if(hx>=0&&hy>=0&&hx<side&&hy<side){
                    float x=lx+(hx+.5f)*step+offsetX,y=ly+(hy+.5f)*step+offsetY;
                    paint.setColor(0xfff5bd79);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(Math.max(2,step*.09f));
                    c.drawRect(x-step*.38f,y-step*.38f,x+step*.38f,y+step*.38f,paint);paint.setStyle(Paint.Style.FILL);
                }
            }
            int px=visualX()-cx+radius,py=visualY()-cy+radius;
            if(px>=0&&py>=0&&px<side&&py<side){
                float x=lx+(px+.5f)*step+offsetX,y=ly+(py+.5f)*step+offsetY;
                paint.setColor(0xff142d2a);c.drawCircle(x,y,step*.35f,paint);
                paint.setColor(0xff80d9cd);c.drawCircle(x,y,step*.23f,paint);
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
            if(travelRoute!=null)return true;
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
