package com.bore2.game;

/** Tile-local articulated 2D poses. Pure math; caller supplies a reused output array. */
final class IndoorPose {
    static final int IDLE=0,WALK=1,WORK=2,EAT=3,COOK=4,REST=5,READ=6,EXERCISE=7,TALK=8,CLEAN=9,SIZE=10;
    static final int BODY_X=0,BODY_Y=1,HEAD_X=2,HEAD_Y=3,LEFT_X=4,LEFT_Y=5,RIGHT_X=6,RIGHT_Y=7,LEFT_FOOT=8,RIGHT_FOOT=9;
    // Stable InteriorPlan, VenueRules and BuildingPlan action IDs.
    static int forAction(int action){
        switch(action){case 0:case 4:case 9:return REST;case 1:return COOK;case 2:case 5:case 7:return WORK;case 3:case 8:return TALK;case 6:return EAT;case 10:return READ;case 11:return EXERCISE;default:return IDLE;}
    }
    static boolean atGoal(int at,int[] goals){if(at<0||at>=324||goals==null)return false;for(int cell:goals)if(at==cell)return true;return false;}
    static void sample(int mode,int facing,float wave,float[] out){
        float w=Float.isFinite(wave)?Math.max(-1f,Math.min(1f,wave)):0;
        int side=facing<0?-1:facing>0?1:0;
        float bx=0,by=0,hx=0,hy=-.25f,lx=-.25f,ly=.12f,rx=.25f,ry=.12f,lf=.31f,rf=.31f;
        switch(mode){
            case WALK:by=-.022f*Math.abs(w);hy+=by;ly+=w*.09f;ry-=w*.09f;lf+=w*.09f;rf-=w*.09f;break;
            case WORK:bx=.025f*side;hx=bx;ly=-.025f+w*.06f;ry=-.035f-w*.06f;lx=-.15f+side*.09f;rx=.15f+side*.09f;break;
            case EAT:bx=.02f*side;hx=bx;lx=-.15f;ly=.01f;rx=.10f;ry=-.15f+w*.075f;if(side<0){lx=-.10f;ly=ry;rx=.15f;ry=.01f;}break;
            case COOK:bx=.04f*side;hx=bx;lx=-.15f+.10f*side;rx=.15f+.10f*side;ly=.04f+w*.10f;ry=-.03f-w*.10f;break;
            case REST:by=.13f;hy=-.12f;lx=-.19f;rx=.19f;ly=ry=.22f;lf=rf=.34f;break;
            case READ:by=.035f;hy=-.20f;lx=-.095f;rx=.095f;ly=.025f;ry=.025f+w*.03f;break;
            case EXERCISE:by=-.035f*Math.abs(w);hy+=by;lx=-.28f;rx=.28f;ly=ry=-.06f-w*.23f;lf+=w*.05f;rf-=w*.05f;break;
            case TALK:rx=.24f;ry=-.04f-w*.12f;if(side<0){lx=-.24f;ly=ry;ry=.12f;}break;
            case CLEAN:bx=.045f*side;by=.04f;hx=bx;hy=-.18f;lx=-.16f+side*.08f;rx=.16f+side*.08f;ly=.19f+w*.05f;ry=.22f-w*.05f;break;
            default:break;
        }
        out[BODY_X]=bx;out[BODY_Y]=by;out[HEAD_X]=hx;out[HEAD_Y]=hy;out[LEFT_X]=lx;out[LEFT_Y]=ly;out[RIGHT_X]=rx;out[RIGHT_Y]=ry;out[LEFT_FOOT]=lf;out[RIGHT_FOOT]=rf;
    }
    private IndoorPose(){}
}
