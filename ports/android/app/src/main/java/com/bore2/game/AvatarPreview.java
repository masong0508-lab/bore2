package com.bore2.game;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.View;
import java.util.Arrays;
import java.util.Comparator;

/** Actual avatar triangles, projected and depth sorted for the character editor. */
final class AvatarPreview extends View {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path=new Path();
    private float[] model,projected;
    private Integer[] order;
    AvatarPreview(Context context,AvatarStyle appearance){super(context);setContentDescription("Character model preview");appearance(appearance);}
    void appearance(AvatarStyle appearance){
        model=AvatarMesh.create(appearance,true);projected=new float[model.length/7*3];
        float cs=.9396926f,sn=.3420201f,cp=.9902681f,sp=.1391731f;
        for(int i=0,j=0;i<model.length;i+=7,j+=3){
            float x=model[i],y=model[i+1],z=model[i+2],depth=sn*x+cs*z;
            projected[j]=cs*x-sn*z;projected[j+1]=cp*y-sp*depth;projected[j+2]=sp*y+cp*depth;
        }
        order=new Integer[model.length/21];for(int i=0;i<order.length;i++)order[i]=i;
        Arrays.sort(order,Comparator.comparingDouble(i->projected[i*9+2]+projected[i*9+5]+projected[i*9+8]));
        invalidate();
    }
    @Override protected void onDraw(Canvas c){
        c.drawColor(0xff263c40);if(order==null)return;
        float s=Math.min(getWidth()*.72f,getHeight()*.46f),x=getWidth()*.5f,y=getHeight()*.91f;
        paint.setColor(0xff192c31);c.drawOval(x-s*.45f,y-s*.06f,x+s*.45f,y+s*.08f,paint);
        for(int k:order){
            int p=k*9,m=k*21;
            path.reset();path.moveTo(x+projected[p]*s,y-projected[p+1]*s);
            path.lineTo(x+projected[p+3]*s,y-projected[p+4]*s);path.lineTo(x+projected[p+6]*s,y-projected[p+7]*s);path.close();
            paint.setColor(Color.rgb(Math.round(model[m+3]*255),Math.round(model[m+4]*255),Math.round(model[m+5]*255)));
            c.drawPath(path,paint);
        }
    }
}
