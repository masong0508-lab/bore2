package com.bore2.game;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

/** A depth-buffered, on-demand renderer for the native BORE neighborhood mesh. */
final class WorldView3D extends GLSurfaceView {
    interface Picker { void onPick(int x,int y,int tileX,int tileY,int originX,int originY,int revision); }
    private final SceneRenderer scene;
    private final Handler ui=new Handler(Looper.getMainLooper());
    private final ScaleGestureDetector scale;
    private float startX,startY,lastX,lastY;
    private boolean dragged,multiple;
    WorldView3D(Context context,Picker pick){
        super(context);
        setEGLContextClientVersion(2);
        setEGLConfigChooser((egl,display)->chooseDepthConfig(egl,display));
        setPreserveEGLContextOnPause(true);
        scene=new SceneRenderer(pick);
        setRenderer(scene);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
        setClickable(true);setFocusable(true);
        scale=new ScaleGestureDetector(context,new ScaleGestureDetector.SimpleOnScaleGestureListener(){
            @Override public boolean onScale(ScaleGestureDetector detector){
                float factor=detector.getScaleFactor();
                if(Float.isFinite(factor)&&factor>0)zoomBy(1f/factor);
                multiple=true;return true;
            }
        });
    }
    void setScene(float[] v,int x,int y,int revision,Runnable ready,Runnable failed){
        queueEvent(()->scene.setScene(v,x,y,revision,ready,failed));requestRender();
    }
    void select(int x,int y){
        queueEvent(()->{scene.selX=x;scene.selY=y;});requestRender();
    }
    void player(int x,int y){playerTile(x*18+2,y*18+2,0);}
    void playerTile(int x,int y,int ms){queueEvent(()->scene.movePlayer(x,y,ms));requestRender();}
    void followPlayer(boolean follow){queueEvent(()->{scene.follow=follow;if(follow)scene.distance=22;});requestRender();}
    void residents(int[] people,int revision){queueEvent(()->scene.setResidents(people,revision));requestRender();}
    void zoomBy(float factor){
        queueEvent(()->scene.distance=Math.max(10,Math.min(105,scene.distance*factor)));requestRender();
    }
    void resetCamera(){
        queueEvent(()->{scene.yaw=42;scene.pitch=49;scene.distance=48;});requestRender();
    }
    @Override public boolean onTouchEvent(MotionEvent event){
        scale.onTouchEvent(event);
        float x=event.getX(),y=event.getY();
        switch(event.getActionMasked()){
            case MotionEvent.ACTION_DOWN:
                startX=lastX=x;startY=lastY=y;dragged=multiple=false;return true;
            case MotionEvent.ACTION_POINTER_DOWN:
            case MotionEvent.ACTION_POINTER_UP:
                multiple=true;dragged=true;lastX=x;lastY=y;return true;
            case MotionEvent.ACTION_MOVE:
                float dx=x-lastX,dy=y-lastY;lastX=x;lastY=y;
                float slop=6*getResources().getDisplayMetrics().density;
                if(Math.abs(x-startX)>slop||Math.abs(y-startY)>slop)dragged=true;
                if(dragged&&!multiple&&!scale.isInProgress()){
                    queueEvent(()->{scene.yaw=(scene.yaw-dx*.32f)%360;scene.pitch=Math.max(24,Math.min(76,scene.pitch+dy*.23f));});
                    requestRender();
                }return true;
            case MotionEvent.ACTION_UP:
                if(!dragged&&!multiple){
                    queueEvent(()->scene.pick(x,y));performClick();
                }return true;
            case MotionEvent.ACTION_CANCEL:dragged=multiple=true;return true;
            default:return true;
        }
    }
    @Override public boolean performClick(){super.performClick();return true;}

    private static EGLConfig chooseDepthConfig(EGL10 egl,EGLDisplay display){
        // Prefer 24-bit depth on devices that support it; retain RGB565/16
        // fallback instead of preventing older phones from starting.
        int[][] choices={{8,8,8,24},{8,8,8,16},{5,6,5,16}};
        for(int[] c:choices){
            int[] attrs={EGL10.EGL_RED_SIZE,c[0],EGL10.EGL_GREEN_SIZE,c[1],EGL10.EGL_BLUE_SIZE,c[2],
                EGL10.EGL_DEPTH_SIZE,c[3],0x3040,4,EGL10.EGL_SURFACE_TYPE,4,EGL10.EGL_NONE};
            EGLConfig[] cfg=new EGLConfig[1];int[] n=new int[1];
            if(egl.eglChooseConfig(display,attrs,cfg,1,n)&&n[0]>0)return cfg[0];
        }
        throw new IllegalArgumentException("No GLES2 configuration with a depth buffer");
    }
    private final class SceneRenderer implements Renderer {
        private static final String VS=
            "attribute vec3 aPosition;attribute vec3 aColor;uniform mat4 uVP;uniform vec3 uEye;uniform vec3 uOffset;uniform vec3 uPose;uniform vec3 uTint;"+
            "varying mediump vec3 vColor;varying mediump float vDist;varying mediump vec2 vXZ;"+
            "void main(){vec3 p=aPosition;if(uPose.x>.5){float swing=uPose.y;float side=p.x<0.?-1.:1.;"+
            "if(p.y<.73)p.z+=side*swing*max(0.,.72-p.y);else if(abs(p.x)>.265)p.z-=side*swing*max(0.,1.33-p.y);"+
            "float cs=cos(uPose.z),sn=sin(uPose.z);p.xz=vec2(cs*p.x+sn*p.z,-sn*p.x+cs*p.z);}p+=uOffset;gl_Position=uVP*vec4(p,1.);vColor=aColor*uTint;vDist=length(p-uEye);vXZ=p.xz;}";
        private static final String FS=
            "precision mediump float;varying mediump vec3 vColor;varying mediump float vDist;varying mediump vec2 vXZ;"+
            "uniform vec2 uSelection;uniform float uSelected;uniform vec3 uSky;uniform vec2 uPlayer;uniform float uPlayerVisible;"+
            "void main(){vec2 q=abs(vXZ-uSelection);float sel=uSelected*(1.-step(9.,max(q.x,q.y)));"+
            "vec3 c=mix(vColor,vec3(.99,.85,.49),sel*.14);vec2 p=abs(vXZ-uPlayer);float d=max(p.x,p.y);float edge=uPlayerVisible*step(7.9,d)*(1.-step(9.,d));c=mix(c,vec3(.35,.95,.80),edge*.7);float f=smoothstep(85.,172.,vDist);"+
            "gl_FragColor=vec4(mix(c,uSky,f),1.);}";
        private final Picker picker;
        private final float[] projection=new float[16],view=new float[16],vp=new float[16],inverse=new float[16];
        private float[] mesh=new float[0],pendingMesh;
        private int pendingX,pendingY,pendingRevision,revision;
        private Runnable pendingReady,pendingFailure,activeFailure;
        private int originX,originY,selX=Integer.MIN_VALUE,selY,width,height,program,vbo;
        private int aPosition,aColor,uVP,uEye,uSelection,uSelected,uSky,uPlayer,uPlayerVisible,uOffset,uPose,uTint;
        private final float[] actor=ActorMesh.create();
        private FloatBuffer actorBuffer;
        private int[] residents=new int[0];
        private final float[] residentY=new float[16];
        private int residentsRevision=-1,residentGroundRevision=-1;
        void setResidents(int[] people,int id){
            if(people==null||people.length%3!=0||people.length>48)return;
            if(residentsRevision==id&&java.util.Arrays.equals(residents,people))return;
            residents=people;residentsRevision=id;residentGroundRevision=-1;
        }
        private int actorX=-1,actorY=-1,actorRevision=-1,actorFromX=-1,actorFromY=-1;
        private float groundFrom=Float.POSITIVE_INFINITY,groundTo=Float.POSITIVE_INFINITY;
        private int playerX=-1,playerY=-1,fromX=-1,fromY=-1;
        private long stepStart;
        private int stepMs;
        private boolean follow;
        private float actorPX,actorPZ,actorPY,swing,facing;
        private boolean actorVisible,animating;
        void movePlayer(int x,int y,int ms){
            if(x==playerX&&y==playerY&&ms==0){fromX=x;fromY=y;stepMs=0;return;}
            boolean adjacent=playerX>=0&&Math.abs(x-playerX)+Math.abs(y-playerY)==1;
            fromX=adjacent&&ms>0?playerX:x;fromY=adjacent&&ms>0?playerY:y;
            if(adjacent&&ms>0)facing=(float)Math.atan2(x-playerX,y-playerY);
            playerX=x;playerY=y;stepMs=adjacent?ms:0;stepStart=SystemClock.uptimeMillis();
        }
        private void actorPose(){
            actorVisible=playerX>=0&&playerY>=0&&Math.abs(playerX/18-originX)<=4&&Math.abs(playerY/18-originY)<=4;
            animating=false;if(!actorVisible)return;
            int baseX=originX*18+9,baseY=originY*18+9;
            if(actorRevision!=revision||actorX!=playerX||actorY!=playerY||actorFromX!=fromX||actorFromY!=fromY){
                float old=groundTo;
                boolean reuse=actorRevision==revision&&actorX==fromX&&actorY==fromY;
                groundFrom=reuse?old:ActorMesh.ground(mesh,fromX-baseX+.5f,fromY-baseY+.5f);
                groundTo=fromX==playerX&&fromY==playerY?groundFrom:ActorMesh.ground(mesh,playerX-baseX+.5f,playerY-baseY+.5f);
                actorX=playerX;actorY=playerY;actorFromX=fromX;actorFromY=fromY;actorRevision=revision;
            }
            if(!Float.isFinite(groundFrom)||!Float.isFinite(groundTo)){actorVisible=false;return;}
            float t=stepMs>0?Math.min(1f,Math.max(0f,(SystemClock.uptimeMillis()-stepStart)/(float)stepMs)):1f;
            actorPX=fromX-baseX+.5f+(playerX-fromX)*t;actorPZ=fromY-baseY+.5f+(playerY-fromY)*t;
            actorPY=groundFrom+(groundTo-groundFrom)*t;
            animating=t<1f;swing=0f;
            if(animating)swing=(float)Math.sin(t*Math.PI)*(((playerX+playerY)&1)==0?.48f:-.48f);
        }
        private boolean dirty=true,validMatrix;
        private FloatBuffer upload; // reuse direct staging storage between neighborhood meshes
        private float targetY=0;
        float yaw=42,pitch=49,distance=48;
        SceneRenderer(Picker pick){picker=pick;}
        void setScene(float[] v,int x,int y,int id,Runnable ready,Runnable failed){
            if(v==null||v.length==0||v.length%18!=0){if(failed!=null)ui.post(failed);return;}
            pendingMesh=v;pendingX=x;pendingY=y;pendingRevision=id;
            pendingReady=ready;pendingFailure=failed;dirty=true;
        }
        private void uploadScene(){
            float[] next=pendingMesh!=null?pendingMesh:mesh;
            if(next.length==0){dirty=false;return;}
            int[] buffers=new int[1];boolean ok=false;
            // Never replace a valid GPU buffer until the new upload succeeds.
            // The second buffer exists only for the duration of a scene swap.
            try{
                if(upload==null||upload.capacity()<next.length)
                    upload=ByteBuffer.allocateDirect(next.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
                upload.clear();upload.put(next);upload.flip();
                for(int i=0;i<8&&GLES20.glGetError()!=GLES20.GL_NO_ERROR;i++){}
                GLES20.glGenBuffers(1,buffers,0);
                GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER,buffers[0]);
                GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER,next.length*4,upload,GLES20.GL_STATIC_DRAW);
                ok=buffers[0]!=0&&GLES20.glGetError()==GLES20.GL_NO_ERROR;
            }catch(OutOfMemoryError ex){ok=false;}
            if(ok){
                if(vbo!=0)GLES20.glDeleteBuffers(1,new int[]{vbo},0);
                vbo=buffers[0];
                if(pendingMesh!=null){
                    mesh=next;originX=pendingX;originY=pendingY;revision=pendingRevision;
                    activeFailure=pendingFailure;validMatrix=false;
                    float low=Float.POSITIVE_INFINITY;
                    for(int i=0;i<mesh.length;i+=6)
                        if(Math.abs(mesh[i])<=10&&Math.abs(mesh[i+2])<=10)low=Math.min(low,mesh[i+1]);
                    targetY=Float.isFinite(low)?low+1f:1f;
                    if(pendingReady!=null)ui.post(pendingReady);
                }
            }else{
                if(buffers[0]!=0)GLES20.glDeleteBuffers(1,buffers,0);
                Runnable failure=pendingMesh!=null?pendingFailure:activeFailure;
                if(failure!=null)ui.post(failure);
            }
            pendingMesh=null;pendingReady=null;pendingFailure=null;dirty=false;
        }
        private int shader(int type,String source){
            int id=GLES20.glCreateShader(type);GLES20.glShaderSource(id,source);GLES20.glCompileShader(id);
            int[] ok=new int[1];GLES20.glGetShaderiv(id,GLES20.GL_COMPILE_STATUS,ok,0);
            if(ok[0]==0)throw new IllegalStateException(GLES20.glGetShaderInfoLog(id));
            return id;
        }
        @Override public void onSurfaceCreated(GL10 gl,EGLConfig cfg){
            int vs=shader(GLES20.GL_VERTEX_SHADER,VS),fs=shader(GLES20.GL_FRAGMENT_SHADER,FS);
            program=GLES20.glCreateProgram();GLES20.glAttachShader(program,vs);GLES20.glAttachShader(program,fs);GLES20.glLinkProgram(program);
            int[] ok=new int[1];GLES20.glGetProgramiv(program,GLES20.GL_LINK_STATUS,ok,0);
            GLES20.glDeleteShader(vs);GLES20.glDeleteShader(fs);
            if(ok[0]==0)throw new IllegalStateException(GLES20.glGetProgramInfoLog(program));
            aPosition=GLES20.glGetAttribLocation(program,"aPosition");aColor=GLES20.glGetAttribLocation(program,"aColor");
            uVP=GLES20.glGetUniformLocation(program,"uVP");uEye=GLES20.glGetUniformLocation(program,"uEye");
            uSelection=GLES20.glGetUniformLocation(program,"uSelection");uSelected=GLES20.glGetUniformLocation(program,"uSelected");
            uSky=GLES20.glGetUniformLocation(program,"uSky");
            uPlayer=GLES20.glGetUniformLocation(program,"uPlayer");uPlayerVisible=GLES20.glGetUniformLocation(program,"uPlayerVisible");
            uOffset=GLES20.glGetUniformLocation(program,"uOffset");uPose=GLES20.glGetUniformLocation(program,"uPose");uTint=GLES20.glGetUniformLocation(program,"uTint");
            if(actorBuffer==null){actorBuffer=ByteBuffer.allocateDirect(actor.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();actorBuffer.put(actor).flip();}
            vbo=0;dirty=true;validMatrix=false;actorRevision=-1;
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);GLES20.glDepthFunc(GLES20.GL_LEQUAL);
            GLES20.glDisable(GLES20.GL_BLEND);GLES20.glDisable(GLES20.GL_CULL_FACE);
        }
        @Override public void onSurfaceChanged(GL10 gl,int w,int h){
            width=w;height=h;GLES20.glViewport(0,0,w,h);validMatrix=false;
        }
        @Override public void onDrawFrame(GL10 gl){
            GLES20.glClearColor(.68f,.77f,.80f,1);GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
            if(width<=0||height<=0)return;
            if(dirty)uploadScene();
            if(vbo==0||mesh.length==0)return;
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER,vbo);
            actorPose();
            float tx=follow&&actorVisible?actorPX:0,tz=follow&&actorVisible?actorPZ:0,ty=follow&&actorVisible?actorPY+1f:targetY;
            float yr=(float)Math.toRadians(yaw),pr=(float)Math.toRadians(pitch);
            float d=distance*(float)Math.cos(pr),ex=tx+d*(float)Math.sin(yr),ez=tz+d*(float)Math.cos(yr);
            float ey=ty+distance*(float)Math.sin(pr);
            float aspect=width/(float)height;
            Matrix.perspectiveM(projection,0,46,aspect,2.5f,220);
            Matrix.setLookAtM(view,0,ex,ey,ez,tx,ty,tz,0,1,0);
            Matrix.multiplyMM(vp,0,projection,0,view,0);
            validMatrix=Matrix.invertM(inverse,0,vp,0);
            GLES20.glUseProgram(program);
            GLES20.glUniformMatrix4fv(uVP,1,false,vp,0);GLES20.glUniform3f(uOffset,0,0,0);GLES20.glUniform3f(uPose,0,0,0);GLES20.glUniform3f(uTint,1,1,1);
            GLES20.glUniform3f(uEye,ex,ey,ez);GLES20.glUniform3f(uSky,.68f,.77f,.80f);
            boolean selected=Math.abs((long)selX-originX)<=4&&Math.abs((long)selY-originY)<=4;
            GLES20.glUniform1f(uSelected,selected?1:0);
            GLES20.glUniform2f(uSelection,selected?(selX-originX)*18f:0,selected?(selY-originY)*18f:0);
            boolean playerHere=actorVisible;
            GLES20.glUniform1f(uPlayerVisible,playerHere&&!follow?1:0);
            GLES20.glUniform2f(uPlayer,playerHere?(playerX/18-originX)*18f:0,playerHere?(playerY/18-originY)*18f:0);
            GLES20.glEnableVertexAttribArray(aPosition);GLES20.glEnableVertexAttribArray(aColor);
            GLES20.glVertexAttribPointer(aPosition,3,GLES20.GL_FLOAT,false,24,0);
            GLES20.glVertexAttribPointer(aColor,3,GLES20.GL_FLOAT,false,24,12);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,mesh.length/6);
            if(playerHere)drawActor();
            drawResidents();
            if(animating)WorldView3D.this.requestRender();
            GLES20.glDisableVertexAttribArray(aPosition);GLES20.glDisableVertexAttribArray(aColor);
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER,0);
        }
        private void bindActor(){
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER,0);
            GLES20.glUniform1f(uPlayerVisible,0);GLES20.glUniform1f(uSelected,0);
            actorBuffer.position(0);GLES20.glVertexAttribPointer(aPosition,3,GLES20.GL_FLOAT,false,24,actorBuffer);
            actorBuffer.position(3);GLES20.glVertexAttribPointer(aColor,3,GLES20.GL_FLOAT,false,24,actorBuffer);
            actorBuffer.position(0);
        }
        private void drawActor(){
            if(!actorVisible||actorBuffer==null)return;
            bindActor();GLES20.glUniform3f(uOffset,actorPX,actorPY+.035f,actorPZ);
            GLES20.glUniform3f(uPose,1,swing,facing);GLES20.glUniform3f(uTint,1,1,1);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,actor.length/6);
        }
        private void drawResidents(){
            if(residentsRevision!=revision||residents.length==0||actorBuffer==null)return;
            int baseX=originX*18+9,baseY=originY*18+9;
            if(residentGroundRevision!=revision){
                for(int i=0;i<residents.length;i+=3)
                    residentY[i/3]=ActorMesh.ground(mesh,residents[i]-baseX+.5f,residents[i+1]-baseY+.5f);
                residentGroundRevision=revision;
            }
            bindActor();
            for(int i=0;i<residents.length;i+=3){
                float x=residents[i]-baseX+.5f,z=residents[i+1]-baseY+.5f,h=residentY[i/3];
                if(!Float.isFinite(h)||(actorVisible&&Math.abs(x-actorPX)<.7f&&Math.abs(z-actorPZ)<.7f))continue;
                int key=residents[i+2],style=ResidentSchedule.mix(key);
                GLES20.glUniform3f(uOffset,x,h+.035f,z);
                GLES20.glUniform3f(uPose,1,0,(style&3)*1.5707963f);
                GLES20.glUniform3f(uTint,.80f+((style>>>2)&3)*.07f,.80f+((style>>>4)&3)*.07f,.80f+((style>>>6)&3)*.07f);
                GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,actor.length/6);
            }
        }
        // Pick the nearest actual visible triangle, including roof and wall
        // surfaces, not a guessed coordinate on a flat ground plane.
        void pick(float sx,float sy){
            if(!validMatrix||mesh.length==0||width<=0||height<=0)return;
            float nx=2*sx/width-1,ny=1-2*sy/height;
            float[] n={nx,ny,-1,1},f={nx,ny,1,1},a=new float[4],b=new float[4];
            Matrix.multiplyMV(a,0,inverse,0,n,0);Matrix.multiplyMV(b,0,inverse,0,f,0);
            if(Math.abs(a[3])<1e-7f||Math.abs(b[3])<1e-7f)return;
            for(int j=0;j<3;j++){a[j]/=a[3];b[j]/=b[3];}
            float dx=b[0]-a[0],dy=b[1]-a[1],dz=b[2]-a[2],best=1.00001f;
            for(int i=0;i<mesh.length;i+=18){
                float x=mesh[i],y=mesh[i+1],z=mesh[i+2];
                float ux=mesh[i+6]-x,uy=mesh[i+7]-y,uz=mesh[i+8]-z;
                float vx=mesh[i+12]-x,vy=mesh[i+13]-y,vz=mesh[i+14]-z;
                float px=dy*vz-dz*vy,py=dz*vx-dx*vz,pz=dx*vy-dy*vx;
                float det=ux*px+uy*py+uz*pz;
                if(Math.abs(det)<1e-7f)continue;
                float inv=1/det,tx=a[0]-x,ty=a[1]-y,tz=a[2]-z;
                float u=(tx*px+ty*py+tz*pz)*inv;if(u<0||u>1)continue;
                float qx=ty*uz-tz*uy,qy=tz*ux-tx*uz,qz=tx*uy-ty*ux;
                float v=(dx*qx+dy*qy+dz*qz)*inv;if(v<0||u+v>1)continue;
                float t=(vx*qx+vy*qy+vz*qz)*inv;if(t>=0&&t<best)best=t;
            }
            if(best>1)return;
            int tx=originX*18+9+(int)Math.floor(a[0]+best*dx),ty=originY*18+9+(int)Math.floor(a[2]+best*dz);
            int ox=originX,oy=originY,id=revision;
            if(OutdoorPath.valid(tx,ty))ui.post(()->picker.onPick(tx/18,ty/18,tx,ty,ox,oy,id));
        }
    }
}
