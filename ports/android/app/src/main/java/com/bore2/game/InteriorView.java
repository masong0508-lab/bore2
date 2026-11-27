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
    interface Listener {void arrived(int action,int cell);void message(String message);void moveFurniture(int slot,int cell);void talkResident(int key);}
    static final int WALK=-1,EXIT=-2,UP=-3,DOWN=-4,TALK=-5;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Listener listener;
    private InteriorPlan plan;
    private IndoorCrowd crowd;
    private AvatarStyle[] peopleStyles=new AvatarStyle[0];
    private int targetPerson=-1;
    private int crowdSeed;
    private int[] crowdKeys=new int[0],crowdRoles=new int[0];
    private int[][] crowdStations,crowdGoals;
    private int[] crowdSlots=new int[0];
    private final float[] poseBuffer=new float[IndoorPose.SIZE];
    private long playerPoseUntil,npcPoseUntil;
    private int playerPose=IndoorPose.IDLE,playerFacing;
    private static final long ACTIVITY_MS=1200;
    private final Runnable poseTick=this::tickPose;
    private boolean poseVisible(){return isShown()&&hasWindowFocus()&&!busy()&&!decorating&&plan!=null;}
    private void tickPose(){
        invalidate();long now=SystemClock.uptimeMillis();
        if(poseVisible()&&(playerPoseUntil>now||(motionEnabled&&npcPoseUntil>now)))postDelayed(poseTick,80);
    }
    private void wakePose(){removeCallbacks(poseTick);if(poseVisible())tickPose();}
    private void stopPose(){removeCallbacks(poseTick);playerPoseUntil=npcPoseUntil=0;playerPose=IndoorPose.IDLE;invalidate();}
    private int facingStation(int cell,int slot){
        if(plan==null||slot<0||slot>=plan.actionCount())return 1;
        int target=plan.objects[slot],dx=target%18-cell%18;return dx==0?1:Integer.signum(dx);
    }
    private int stationPose(int slot){
        if(slot<0||plan==null)return IndoorPose.IDLE;
        int action=plan.actionAt(slot);
        if(action==VenueRules.ACTIVITY){if(plan.lot==8)return IndoorPose.EXERCISE;if(plan.lot==27)return IndoorPose.READ;}
        return IndoorPose.forAction(action);
    }
    void playActivity(int mode,int service){
        if(plan==null||busy()||decorating)return;
        int slot=plan.slotFor(service);playerFacing=facingStation(at,slot);
        playerPose=mode;playerPoseUntil=SystemClock.uptimeMillis()+ACTIVITY_MS;wakePose();
    }
    private void animateArrivals(){
        if(!motionEnabled||crowd==null||crowdGoals==null)return;
        for(int i=0;i<crowd.size();i++)if(IndoorPose.atGoal(crowd.cell(i),crowdGoals[i])){
            npcPoseUntil=SystemClock.uptimeMillis()+ACTIVITY_MS;wakePose();return;
        }
    }
    private boolean motionEnabled;
    private int motionCounter;
    private IndoorCrowd.Step moving;
    private long motionStart;
    private static final long NPC_STEP_MS=320;
    private final Runnable motionTick=this::moveResident;
    private boolean canAnimate(){
        return motionEnabled&&!decorating&&plan!=null&&crowd!=null&&crowd.size()>0&&!busy()&&isShown()&&hasWindowFocus();
    }
    private void stopMotion(){removeCallbacks(motionTick);moving=null;invalidate();}
    private void restartMotion(){
        stopMotion();if(canAnimate())postDelayed(motionTick,550);
    }
    void setMotionEnabled(boolean enabled){
        if(motionEnabled!=enabled){motionEnabled=enabled;if(!enabled){npcPoseUntil=0;wakePose();}restartMotion();if(enabled)animateArrivals();}
    }
    private float motionProgress(){
        return Math.min(1f,Math.max(0f,(SystemClock.uptimeMillis()-motionStart)/(float)NPC_STEP_MS));
    }
    private void moveResident(){
        if(!canAnimate()){stopMotion();return;}
        if(moving!=null){
            if(motionProgress()<1f){invalidate();postDelayed(motionTick,80);return;}
            int arrived=moving.index;moving=null;invalidate();
            if(IndoorPose.atGoal(crowd.cell(arrived),crowdGoals[arrived]))animateArrivals();
            postDelayed(motionTick,550);return;
        }
        moving=crowd.advanceToward(at,plan.entry,landing,crowdStations,crowdGoals,motionCounter++);
        if(moving!=null){motionStart=SystemClock.uptimeMillis();invalidate();postDelayed(motionTick,80);}
        // Everyone arrived or is blocked. Wake only for new goals, player
        // movement, window focus or a new population snapshot.
    }
    @Override public void onWindowFocusChanged(boolean focused){
        super.onWindowFocusChanged(focused);if(!focused)stopPose();else wakePose();restartMotion();
    }
    @Override protected void onVisibilityChanged(View changed,int visibility){
        super.onVisibilityChanged(changed,visibility);
        if(motionTick!=null){if(!isShown())stopPose();else wakePose();restartMotion();}
    }
    private int[] stationGoals(int slot){
        int[] out=new int[4];for(int d=0;d<4;d++){int cell=InteriorPlan.neighbor(plan.objects[slot],d);out[d]=plan.canUse(cell,slot)?cell:-1;}return out;
    }
    void setPeople(int seed,int[] ids,int[] roles){
        if(plan==null||busy())return;
        if(decorating||ids==null||ids.length==0||roles==null||roles.length!=ids.length){
            stopMotion();crowd=null;crowdKeys=new int[0];crowdRoles=new int[0];crowdStations=crowdGoals=null;peopleStyles=new AvatarStyle[0];invalidate();return;
        }
        if(crowd!=null&&crowdSeed==seed&&java.util.Arrays.equals(ids,crowdKeys)){
            if(!java.util.Arrays.equals(roles,crowdRoles)){
                crowdRoles=roles.clone();refreshGoals();restartMotion();animateArrivals();
            }
            return;
        }
        stopMotion();crowdSeed=seed;crowdKeys=ids.clone();crowdRoles=roles.clone();
        boolean[] floor=new boolean[InteriorPlan.N];for(int i=0;i<floor.length;i++)floor[i]=plan.walkable(i);
        int[][] stations=new int[plan.actionCount()][];for(int k=0;k<stations.length;k++)stations[k]=stationGoals(k);
        int slot=plan.slotFor(plan.commercial?VenueRules.MEET:InteriorPlan.SOFA);
        int preferred=slot>=0?plan.objects[slot]:plan.entry;
        crowdStations=stations;crowd=IndoorCrowd.create(floor,plan.walls,stations,plan.entry,landing,at,ids,preferred);
        peopleStyles=new AvatarStyle[crowd.size()];for(int i=0;i<peopleStyles.length;i++)peopleStyles[i]=AvatarStyle.resident(seed,crowd.id(i));
        refreshGoals();restartMotion();animateArrivals();
    }
    private void refreshGoals(){
        if(crowd==null)return;
        crowdGoals=new int[crowd.size()][];crowdSlots=new int[crowd.size()];
        for(int i=0;i<crowd.size();i++){
            int slot=-1;
            for(int k=0;k<crowdKeys.length;k++)if(crowdKeys[k]==crowd.id(i)){slot=plan.slotFor(crowdRoles[k]);break;}
            if(slot<0)slot=plan.slotFor(plan.commercial?VenueRules.MEET:InteriorPlan.SOFA);
            if(slot<0)slot=plan.slotFor(plan.commercial?VenueRules.WAIT:InteriorPlan.HOBBY);
            crowdSlots[i]=slot;crowdGoals[i]=slot>=0?stationGoals(slot):new int[0];
        }
    }
    boolean nearResident(int id){return !decorating&&!busy()&&crowd!=null&&crowd.adjacent(at,id);}
    private int[] route(int cell){return crowd==null?plan.route(at,cell):crowd.route(at,cell);}
    private boolean decorating;
    private int selected=-1,theme,upgrades,dirt;
    void setDirt(int value){dirt=Math.max(0,Math.min(100,value));invalidate();}
    void setUpgrades(int mask){upgrades=mask&7;invalidate();}
    void setTheme(int value){theme=value;invalidate();}
    int selectedFurniture(){return selected;}
    void setDecorating(boolean active){
        if(decorating!=active){decorating=active;selected=-1;if(active)stopPose();restartMotion();}
        setContentDescription(active?"Decorate home. Select furniture, then choose a free floor tile.":plan!=null&&plan.commercial?"Venue cutaway. Tap a floor tile or service station.":"Home cutaway. Tap a floor tile to walk or furniture to interact.");invalidate();
    }
    private AvatarStyle style=AvatarStyle.defaults();
    void appearance(AvatarStyle next){style=next;invalidate();}
    private int at,action=WALK,index,lo,hi,top,bottom,landing=-1;
    private int[] path;
    private long started;
    private float tile,left,upper;
    private static final long STEP_MS=145;
    private final Runnable tick=this::step;
    InteriorView(Context context,Listener listener){
        super(context);this.listener=listener;setClickable(true);setFocusable(true);
        setContentDescription("Home cutaway. Tap a floor tile to walk or furniture to interact.");
    }
    void open(InteriorPlan p,int saved,int stair){
        cancel();stopMotion();plan=p;crowd=null;crowdKeys=new int[0];crowdRoles=new int[0];crowdStations=crowdGoals=null;motionCounter=0;peopleStyles=new AvatarStyle[0];decorating=false;selected=-1;landing=stair;at=p.walkable(saved)?saved:p.entry;
        setContentDescription(p.commercial?"Venue cutaway. Tap a floor tile or service station.":"Home cutaway. Tap a floor tile to walk or furniture to interact.");
        lo=18;hi=-1;top=18;bottom=-1;
        for(int i=0;i<InteriorPlan.N;i++)if(p.floor(i)){lo=Math.min(lo,i%18);hi=Math.max(hi,i%18);top=Math.min(top,i/18);bottom=Math.max(bottom,i/18);}
        invalidate();
    }
    int cell(){return at;}
    boolean busy(){return path!=null;}
    void cancel(){removeCallbacks(tick);path=null;index=0;action=WALK;targetPerson=-1;stopPose();restartMotion();}
    void close(){cancel();stopMotion();plan=null;crowd=null;crowdKeys=new int[0];crowdRoles=new int[0];crowdStations=crowdGoals=null;peopleStyles=new AvatarStyle[0];decorating=false;selected=-1;}
    void use(int kind){
        if(plan==null||kind<0||kind>=plan.actionCount())return;
        if(decorating){selected=kind;listener.message(plan.labelAt(kind)+" selected");invalidate();return;}
        begin(crowd==null?plan.routeToObject(at,kind):crowd.routeToAny(at,stationGoals(kind)),kind);
    }
    void exit(){if(!decorating&&plan!=null)begin(route(plan.entry),EXIT);}
    void climb(int direction){if(!decorating&&plan!=null&&plan.walkable(landing)&&(direction==1||direction==-1))begin(route(landing),direction==1?UP:DOWN);}
    private void begin(int[] next,int what){begin(next,what,-1);}
    private void begin(int[] next,int what,int person){
        if(next==null){listener.message("No clear path");return;}
        cancel();stopMotion();action=what;targetPerson=person;
        if(next.length==0){finish();return;}
        path=next;started=SystemClock.uptimeMillis();postOnAnimation(tick);invalidate();
    }
    private void finish(){
        int done=action,person=targetPerson;path=null;action=WALK;index=0;targetPerson=-1;
        if(done==TALK){listener.arrived(WALK,at);if(nearResident(person))listener.talkResident(person);restartMotion();return;}
        if(plan!=null&&(done==WALK||(done==EXIT&&at==plan.entry)||((done==UP||done==DOWN)&&at==landing)||(done>=0&&plan.canUse(at,done))))listener.arrived(done,at);
        restartMotion();
    }
    private void step(){
        if(path==null||plan==null)return;
        long now=SystemClock.uptimeMillis();
        while(path!=null&&now-started>=STEP_MS){
            int next=path[index];
            if(!plan.canStep(at,next)||(crowd!=null&&!crowd.canStep(at,next))){cancel();listener.message("Path blocked");return;}
            at=next;index++;started+=STEP_MS;
            if(index==path.length){finish();return;}
        }
        invalidate();if(path!=null)postOnAnimation(tick);
    }
    @Override protected void onDetachedFromWindow(){cancel();stopMotion();super.onDetachedFromWindow();}
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
        switch(plan.actionAt(kind)){
            case VenueRules.WORK:
                rect(c,x+.08f,y+.12f,.84f,.66f,0xff9b724e);rect(c,x+.18f,y+.17f,.45f,.30f,0xff32484b);
                rect(c,x+.23f,y+.21f,.35f,.22f,0xff98c6bf);rect(c,x+.65f,y+.20f,.18f,.38f,0xffe6d7b8);break;
            case VenueRules.MEAL:
                rect(c,x+.04f,y+.08f,.92f,.75f,0xff5d7c80);rect(c,x+.12f,y+.16f,.34f,.52f,0xffb9d0ce);
                rect(c,x+.52f,y+.20f,.36f,.46f,0xffd1a678);rect(c,x+.60f,y+.29f,.20f,.18f,0xfff4e5c2);break;
            case VenueRules.ACTIVITY:
                rect(c,x+.09f,y+.10f,.82f,.80f,0xff60799b);rect(c,x+.18f,y+.16f,.64f,.45f,0xffbdd4dd);
                rect(c,x+.26f,y+.68f,.48f,.14f,0xffd2ba80);break;
            case VenueRules.MEET:
                rect(c,x+.20f,y+.13f,.60f,.72f,0xff8b6c4e);rect(c,x+.30f,y+.20f,.40f,.48f,0xffa8c7b9);break;
            case BuildingPlan.STUDY:
                rect(c,x+.07f,y+.08f,.86f,.78f,0xff8a6347);for(int i=0;i<4;i++)rect(c,x+.17f+i*.17f,y+.17f,.12f,.48f,0xff859ca9+i*0x000a0600);break;
            case BuildingPlan.FITNESS:
                rect(c,x+.12f,y+.08f,.76f,.84f,0xff536d77);rect(c,x+.22f,y+.21f,.56f,.55f,0xffa4c1bd);rect(c,x+.32f,y+.35f,.36f,.24f,0xff515e63);break;
            case InteriorPlan.BED:
                rect(c,x+.08f,y+.05f,.78f,.85f,0xff805a43);rect(c,x+.14f,y+.11f,.66f,.73f,HomeDecor.color(theme,3));
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
            case VenueRules.WAIT:
            case InteriorPlan.SOFA:
                rect(c,x+.08f,y+.11f,.84f,.72f,0xffa9654b);rect(c,x+.17f,y+.23f,.66f,.48f,plan.commercial?0xffd89e71:HomeDecor.color(theme,3));
                rect(c,x+.08f,y+.11f,.84f,.15f,0xff8b523d);break;
            default:break;
        }
        int action=plan.actionAt(kind),bit=action==InteriorPlan.BED?1:action==InteriorPlan.KITCHEN?2:action==InteriorPlan.SOFA?4:0;
        if(!plan.commercial&&(upgrades&bit)!=0){
            rect(c,x+.70f,y+.09f,.20f,.20f,0xffb88b38);
            rect(c,x+.74f,y+.13f,.12f,.12f,0xffffe5a1);
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
            paint.setColor(plan.commercial?(((x+y)&1)==0?0xffb8c4c3:0xffacb8b9):HomeDecor.color(theme,(x+y)&1));
            c.drawRect(X(x),Y(y),X(x+1)+.25f,Y(y+1)+.25f,paint);
            // Sparse, subtle grime marks; never replace a tile or its collision.
            if(!plan.commercial&&dirt>=15&&((i*37+11)%101)<dirt/3){
                paint.setColor(0x284c3824);c.drawOval(X(x+.27f),Y(y+.36f),X(x+.62f),Y(y+.58f),paint);
            }
        }
        paint.setStrokeWidth(Math.max(2,tile*.13f));paint.setStrokeCap(Paint.Cap.SQUARE);
        for(int i=0;i<InteriorPlan.N;i++)if(plan.floor(i)){
            int x=i%18,y=i/18;
            for(int d=0;d<4;d++)if((plan.walls[i]&(1<<d))!=0){
                if(i==plan.entry&&!plan.floor(InteriorPlan.neighbor(i,d))){
                    paint.setColor(0xff4fada0);
                }else paint.setColor(plan.commercial?0xff56473d:HomeDecor.color(theme,2));
                if(d==0)c.drawLine(X(x+1),Y(y),X(x+1),Y(y+1),paint);
                else if(d==1)c.drawLine(X(x),Y(y+1),X(x+1),Y(y+1),paint);
                else if(d==2)c.drawLine(X(x),Y(y),X(x),Y(y+1),paint);
                else c.drawLine(X(x),Y(y),X(x+1),Y(y),paint);
            }
        }
        if(landing>=0){
            float x=landing%18,y=landing/18;rect(c,x+.10f,y+.10f,.80f,.80f,0xff6c8486);
            for(int k=0;k<5;k++)rect(c,x+.17f,y+.18f+k*.12f,.66f,.06f,0xffd2ddd3);
        }
        if(path!=null){
            paint.setColor(0xff688f80);for(int k=index;k<path.length;k++)c.drawCircle(X(path[k]%18+.5f),Y(path[k]/18+.5f),tile*.08f,paint);
        }
        for(int k=0;k<plan.actionCount();k++)furniture(c,k,plan.objects[k]);
        if(decorating&&selected>=0&&selected<plan.actionCount()){
            int q=plan.objects[selected];float x=q%18,y=q/18;
            paint.setColor(0xfff2d77c);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(Math.max(2,tile*.07f));
            c.drawRect(X(x)+2,Y(y)+2,X(x+1)-2,Y(y+1)-2,paint);paint.setStyle(Paint.Style.FILL);
        }
        float px=at%18+.5f,py=at/18+.5f,phase=0;
        if(path!=null){
            float t=Math.min(1f,(SystemClock.uptimeMillis()-started)/(float)STEP_MS);
            int next=path[index];px+=(next%18-at%18)*t;py+=(next/18-at/18)*t;
            phase=(float)Math.sin((index+t)*Math.PI);
        }
        long now=SystemClock.uptimeMillis();
        if(!decorating&&crowd!=null)for(int i=0;i<crowd.size();i++){
            int cell=crowd.cell(i),mode=IndoorPose.IDLE,face=1;
            float x=cell%18+.5f,y=cell/18+.5f,wave=0f;
            if(moving!=null&&moving.index==i){
                float t=motionProgress();int from=moving.from;
                x=from%18+.5f+(cell%18-from%18)*t;y=from/18+.5f+(cell/18-from/18)*t;
                mode=IndoorPose.WALK;face=Integer.signum(cell%18-from%18);
                wave=(float)Math.sin(t*Math.PI)*(((cell+from)&2)==0?1f:-1f);
            }else if(crowdGoals!=null&&IndoorPose.atGoal(cell,crowdGoals[i])){
                mode=stationPose(crowdSlots[i]);face=facingStation(cell,crowdSlots[i]);
                if(motionEnabled&&npcPoseUntil>now)wave=(float)Math.sin((now+(crowd.id(i)&255))*Math.PI/320);
            }
            drawPerson(c,x,y,mode,face,wave,peopleStyles[i],false);
        }
        int mode=path!=null?IndoorPose.WALK:playerPoseUntil>now?playerPose:IndoorPose.IDLE;
        float wave=path!=null?phase:playerPoseUntil>now?(float)Math.sin(now*Math.PI/300):0;
        drawPerson(c,px,py,mode,playerFacing,wave,style,true);
        if(tile>=27){
            for(int k=0;k<plan.actionCount();k++){
                int p=plan.objects[k];text(c,plan.labelAt(k),X(p%18+.5f),Y(p/18+.98f),Math.min(tile*.22f,14),0xff332f2b);
            }
        }
    }
    private void drawArm(Canvas c,float sx,float sy,float hx,float hy,int shirt,int skin){
        float ex=sx+(hx-sx)*.55f,ey=sy+(hy-sy)*.55f;
        paint.setStrokeWidth(.105f);paint.setColor(shirt);c.drawLine(sx,sy,ex,ey,paint);
        paint.setStrokeWidth(.076f);paint.setColor(skin);c.drawLine(ex,ey,hx,hy,paint);
        c.drawCircle(hx,hy,.047f,paint);
    }
    private void drawPerson(Canvas c,float px,float py,int mode,int facing,float wave,AvatarStyle style,boolean player){
        IndoorPose.sample(mode,facing,wave,poseBuffer);
        float[] p=poseBuffer;float bx=p[IndoorPose.BODY_X],by=p[IndoorPose.BODY_Y],hx=p[IndoorPose.HEAD_X],hy=p[IndoorPose.HEAD_Y];
        int skin=0xff000000|style.color(0),hair=0xff000000|style.color(1),shirt=0xff000000|style.color(2),pants=0xff000000|style.color(3);
        c.save();c.translate(X(px),Y(py));c.scale(tile,tile);
        paint.setStyle(Paint.Style.FILL);paint.setColor(0x55302010);c.drawOval(-.27f,.19f,.27f,.40f,paint);
        if(player){paint.setColor(0x9950b7a0);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(.032f);c.drawOval(-.30f,.17f,.30f,.43f,paint);paint.setStyle(Paint.Style.FILL);}
        paint.setStrokeCap(Paint.Cap.ROUND);paint.setStrokeWidth(.115f);paint.setColor(pants);
        c.drawLine(bx-.085f,by+.11f,-.105f,p[IndoorPose.LEFT_FOOT],paint);
        c.drawLine(bx+.085f,by+.11f,.105f,p[IndoorPose.RIGHT_FOOT],paint);
        paint.setColor(0xff353438);
        c.drawRoundRect(-.18f,p[IndoorPose.LEFT_FOOT]-.025f,-.04f,p[IndoorPose.LEFT_FOOT]+.045f,.03f,.03f,paint);
        c.drawRoundRect(.04f,p[IndoorPose.RIGHT_FOOT]-.025f,.18f,p[IndoorPose.RIGHT_FOOT]+.045f,.03f,.03f,paint);
        drawArm(c,bx-.145f,by-.12f,p[IndoorPose.LEFT_X],p[IndoorPose.LEFT_Y],shirt,skin);
        paint.setColor(shirt);c.drawRoundRect(bx-.18f,by-.175f,bx+.18f,by+.15f,.08f,.08f,paint);
        paint.setColor(0x2288ffff);c.drawRoundRect(bx-.11f,by-.13f,bx+.08f,by+.04f,.04f,.04f,paint);
        paint.setColor(skin);c.drawRoundRect(hx-.065f,hy+.08f,hx+.065f,hy+.17f,.03f,.03f,paint);
        c.drawOval(hx-.139f,hy-.151f,hx+.139f,hy+.145f,paint);
        paint.setColor(hair);
        float top=style.choice(4)==1?.205f:style.choice(4)==3?.145f:.17f;
        if(style.choice(4)==2)c.drawRoundRect(hx-.155f,hy-.16f,hx+.155f,hy+.12f,.08f,.08f,paint);
        c.drawArc(hx-.15f,hy-top,hx+.15f,hy+.10f,180,180,true,paint);
        if(tile>=19){
            paint.setColor(0xff33333b);float look=facing<0?-.015f:facing>0?.015f:0;
            c.drawCircle(hx-.048f+look,hy+.015f,.012f,paint);c.drawCircle(hx+.048f+look,hy+.015f,.012f,paint);
        }
        drawArm(c,bx+.145f,by-.12f,p[IndoorPose.RIGHT_X],p[IndoorPose.RIGHT_Y],shirt,skin);
        if(mode==IndoorPose.READ){
            paint.setColor(0xffe5d6af);c.drawRoundRect(-.115f,-.055f,.115f,.045f,.018f,.018f,paint);
            paint.setColor(0xff807659);paint.setStrokeWidth(.016f);c.drawLine(0,-.045f,0,.035f,paint);
        }else if(mode==IndoorPose.CLEAN){
            float x=p[IndoorPose.RIGHT_X],y=p[IndoorPose.RIGHT_Y];paint.setColor(0xff94734d);paint.setStrokeWidth(.028f);c.drawLine(x,y,x+.04f,.37f,paint);
            paint.setColor(0xffd8b971);c.drawRoundRect(x-.05f,.36f,x+.13f,.42f,.02f,.02f,paint);
        }else if(mode==IndoorPose.EAT||mode==IndoorPose.COOK){
            boolean left=mode==IndoorPose.EAT&&facing<0;
            float x=p[left?IndoorPose.LEFT_X:IndoorPose.RIGHT_X],y=p[left?IndoorPose.LEFT_Y:IndoorPose.RIGHT_Y];paint.setColor(0xffe3e6dc);paint.setStrokeWidth(.022f);c.drawLine(x,y,x+.045f,y-.055f,paint);c.drawCircle(x+.045f,y-.055f,.022f,paint);
        }
        c.restore();
    }
    @Override public boolean onTouchEvent(MotionEvent event){
        if(event.getActionMasked()!=MotionEvent.ACTION_UP)return true;
        if(plan==null||tile<=0)return true;
        int x=(int)Math.floor((event.getX()-left)/tile)+lo,y=(int)Math.floor((event.getY()-upper)/tile)+top;
        if(!InteriorPlan.inside(x,y))return true;
        int cell=y*18+x;if(!plan.floor(cell))return true;
        performClick();
        if(decorating){
            if(plan.occupied[cell]>=0)use(plan.occupied[cell]);
            else if(selected>=0)listener.moveFurniture(selected,cell);
            else listener.message("Select a piece of furniture first");
            return true;
        }
        int person=crowd==null?-1:crowd.personAt(cell);
        if(person<0&&moving!=null&&cell==moving.from&&motionProgress()<.5f)person=crowd.id(moving.index);
        if(person>=0){begin(crowd.routeToPerson(at,person),TALK,person);return true;}
        if(plan.occupied[cell]>=0)use(plan.occupied[cell]);else begin(route(cell),WALK);
        return true;
    }
    @Override public boolean performClick(){super.performClick();return true;}
}
