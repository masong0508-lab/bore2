package com.bore2.game;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** A depth-buffered, on-demand renderer for the native BORE neighborhood mesh. */
final class WorldView3D extends GLSurfaceView {
    interface Picker { void onPick(int x,int y,int originX,int originY); }
    private final SceneRenderer scene;
    private final ScaleGestureDetector scale;
    private float startX,startY,lastX,lastY;
    private boolean dragged,multiple;
    WorldView3D(Context context,Picker pick){
        super(context);
        setEGLContextClientVersion(2);
        setEGLConfigChooser(8,8,8,0,16,0);
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
    void setScene(float[] v,int x,int y){
        queueEvent(()->scene.setScene(v,x,y));requestRender();
    }
    void select(int x,int y){
        queueEvent(()->{scene.selX=x;scene.selY=y;});requestRender();
    }
    void player(int x,int y){queueEvent(()->{scene.playerX=x;scene.playerY=y;});requestRender();}
    void zoomBy(float factor){
        queueEvent(()->scene.distance=Math.max(18,Math.min(105,scene.distance*factor)));requestRender();
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

    private final class SceneRenderer implements Renderer {
        private static final String VS=
            "attribute vec3 aPosition;attribute vec3 aColor;uniform mat4 uVP;uniform vec3 uEye;"+
            "varying mediump vec3 vColor;varying mediump float vDist;varying mediump vec2 vXZ;"+
            "void main(){gl_Position=uVP*vec4(aPosition,1.);vColor=aColor;vDist=length(aPosition-uEye);vXZ=aPosition.xz;}";
        private static final String FS=
            "precision mediump float;varying mediump vec3 vColor;varying mediump float vDist;varying mediump vec2 vXZ;"+
            "uniform vec2 uSelection;uniform float uSelected;uniform vec3 uSky;uniform vec2 uPlayer;uniform float uPlayerVisible;"+
            "void main(){vec2 q=abs(vXZ-uSelection);float sel=uSelected*(1.-step(9.,max(q.x,q.y)));"+
            "vec3 c=mix(vColor,vec3(.99,.85,.49),sel*.14);vec2 p=abs(vXZ-uPlayer);float d=max(p.x,p.y);float edge=uPlayerVisible*step(7.9,d)*(1.-step(9.,d));c=mix(c,vec3(.35,.95,.80),edge*.7);float f=smoothstep(85.,172.,vDist);"+
            "gl_FragColor=vec4(mix(c,uSky,f),1.);}";
        private final Picker picker;
        private final float[] projection=new float[16],view=new float[16],vp=new float[16],inverse=new float[16];
        private float[] mesh=new float[0];
        private int originX,originY,selX=Integer.MIN_VALUE,selY,width,height,program,vbo;
        private int aPosition,aColor,uVP,uEye,uSelection,uSelected,uSky,uPlayer,uPlayerVisible;
        private int playerX=Integer.MIN_VALUE,playerY=Integer.MIN_VALUE;
        private boolean dirty=true,validMatrix;
        private FloatBuffer upload; // reuse direct staging storage between neighborhood meshes
        private float targetY=0;
        float yaw=42,pitch=49,distance=48;
        SceneRenderer(Picker pick){picker=pick;}
        void setScene(float[] v,int x,int y){
            mesh=v!=null&&v.length%18==0?v:new float[0];originX=x;originY=y;dirty=true;validMatrix=false;
            float low=Float.POSITIVE_INFINITY;
            for(int i=0;i<mesh.length;i+=6)
                if(Math.abs(mesh[i])<=10&&Math.abs(mesh[i+2])<=10)low=Math.min(low,mesh[i+1]);
            targetY=Float.isFinite(low)?low+1.0f:1.0f;
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
            int[] buffers=new int[1];GLES20.glGenBuffers(1,buffers,0);vbo=buffers[0];dirty=true;
            GLES20.glEnable(GLES20.GL_DEPTH_TEST);GLES20.glDepthFunc(GLES20.GL_LEQUAL);
            GLES20.glDisable(GLES20.GL_BLEND);GLES20.glDisable(GLES20.GL_CULL_FACE);
        }
        @Override public void onSurfaceChanged(GL10 gl,int w,int h){
            width=w;height=h;GLES20.glViewport(0,0,w,h);validMatrix=false;
        }
        @Override public void onDrawFrame(GL10 gl){
            GLES20.glClearColor(.68f,.77f,.80f,1);GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT|GLES20.GL_DEPTH_BUFFER_BIT);
            if(width<=0||height<=0||mesh.length==0)return;
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER,vbo);
            if(dirty){
                if(upload==null||upload.capacity()<mesh.length)
                    upload=ByteBuffer.allocateDirect(mesh.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
                upload.clear();upload.put(mesh);upload.flip();
                GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER,mesh.length*4,upload,GLES20.GL_STATIC_DRAW);dirty=false;
            }
            float yr=(float)Math.toRadians(yaw),pr=(float)Math.toRadians(pitch);
            float d=distance*(float)Math.cos(pr),ex=d*(float)Math.sin(yr),ez=d*(float)Math.cos(yr);
            float ey=targetY+distance*(float)Math.sin(pr);
            float aspect=width/(float)height;
            Matrix.perspectiveM(projection,0,46,aspect,1.4f,250);
            Matrix.setLookAtM(view,0,ex,ey,ez,0,targetY,0,0,1,0);
            Matrix.multiplyMM(vp,0,projection,0,view,0);
            validMatrix=Matrix.invertM(inverse,0,vp,0);
            GLES20.glUseProgram(program);
            GLES20.glUniformMatrix4fv(uVP,1,false,vp,0);
            GLES20.glUniform3f(uEye,ex,ey,ez);GLES20.glUniform3f(uSky,.68f,.77f,.80f);
            boolean selected=Math.abs((long)selX-originX)<=4&&Math.abs((long)selY-originY)<=4;
            GLES20.glUniform1f(uSelected,selected?1:0);
            GLES20.glUniform2f(uSelection,selected?(selX-originX)*18f:0,selected?(selY-originY)*18f:0);
            boolean playerHere=Math.abs((long)playerX-originX)<=4&&Math.abs((long)playerY-originY)<=4;
            GLES20.glUniform1f(uPlayerVisible,playerHere?1:0);
            GLES20.glUniform2f(uPlayer,playerHere?(playerX-originX)*18f:0,playerHere?(playerY-originY)*18f:0);
            GLES20.glEnableVertexAttribArray(aPosition);GLES20.glEnableVertexAttribArray(aColor);
            GLES20.glVertexAttribPointer(aPosition,3,GLES20.GL_FLOAT,false,24,0);
            GLES20.glVertexAttribPointer(aColor,3,GLES20.GL_FLOAT,false,24,12);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLES,0,mesh.length/6);
            GLES20.glDisableVertexAttribArray(aPosition);GLES20.glDisableVertexAttribArray(aColor);
            GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER,0);
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
            int x=originX+(int)Math.floor((a[0]+best*dx+9)/18);
            int y=originY+(int)Math.floor((a[2]+best*dz+9)/18);
            int ox=originX,oy=originY;
            if(x>=0&&x<1024&&y>=0&&y<1024)post(()->picker.onPick(x,y,ox,oy));
        }
    }
}
