package com.bore2.game;

import java.util.Arrays;

/** Service roles for indoor mobile venues, matching the native lot IDs. */
final class VenueRules {
    static final int WORK=5,MEAL=6,ACTIVITY=7,MEET=8,WAIT=9;
    static boolean supported(int lot){switch(lot){case 6:case 7:case 8:case 9:case 10:case 12:case 13:case 14:case 15:case 21:case 23:case 25:case 27:case 28:case 29:case 30:case 31:case 33:case 34:return true;default:return false;}}
    static boolean has(int lot,int action){
        if(!supported(lot))return false;
        switch(action){
            case WORK:switch(lot){case 6:case 7:case 8:case 9:case 14:case 21:case 25:case 27:case 29:case 30:case 31:return true;default:return false;}
            case MEAL:switch(lot){case 6:case 7:case 10:case 15:case 21:case 25:case 30:case 31:return true;default:return false;}
            case ACTIVITY:switch(lot){case 8:case 10:case 12:case 13:case 15:case 21:case 23:case 27:case 28:case 29:case 33:case 34:return true;default:return false;}
            case MEET:case WAIT:return true;
            default:return false;
        }
    }
    static int[] actions(int lot){int[] a=new int[5];int n=0;for(int k=WORK;k<=WAIT;k++)if(has(lot,k))a[n++]=k;return Arrays.copyOf(a,n);}
    static String label(int lot,int action){
        switch(action){
            case WORK:return lot==9||lot==14?"Work desk":"Work counter";
            case MEAL:return "Order meal";
            case ACTIVITY:return lot==8?"Exercise":lot==27?"Books":lot==13?"Play games":lot==23?"Cinema":lot==29?"Art":lot==12?"Class": "Activities";
            case MEET:return "Meet locals";
            case WAIT:return "Seat";
            default:return "";
        }
    }
    static boolean mealOpen(int lot,int minute){
        if(!has(lot,MEAL)||minute<0||minute>=1440)return false;
        if(lot==30||lot==31||lot==10||lot==15)return true;
        return minute>=420&&minute+30<=1260;
    }
    private VenueRules(){}
}
