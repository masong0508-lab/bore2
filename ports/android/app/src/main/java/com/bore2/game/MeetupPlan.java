package com.bore2.game;

import java.util.HashMap;
import java.util.function.IntUnaryOperator;

/** A bounded arrangement to meet a known resident at their normal public destination. */
final class MeetupPlan {
    static final int UPCOMING=0,READY=1,MISSED=2;
    static final int MIN_AFFINITY=10,ARRIVAL=30,DURATION=30;
    final int contact,place,day,start;
    interface Schedule {ResidentSchedule.Entry at(int day,int minute);}
    private MeetupPlan(int contact,int place,int day,int start){this.contact=contact;this.place=place;this.day=day;this.start=start;}
    static boolean publicLot(int lot){switch(lot){case 7:case 8:case 10:case 11:case 13:case 15:case 16:case 17:case 18:case 21:case 22:case 23:case 25:case 27:case 28:case 29:case 33:case 34:return true;default:return false;}}
    static boolean available(ResidentSchedule.Entry e,int contact,int place){return e!=null&&e.home==contact&&e.location==place&&(e.phase==ResidentSchedule.LEISURE||e.phase==ResidentSchedule.LUNCH);}
    int state(int nowDay,int minute){if(nowDay<day||(nowDay==day&&minute<start))return UPCOMING;return nowDay==day&&minute<=start+ARRIVAL?READY:MISSED;}
    boolean completeAt(int person,int location,int nowDay,int minute){return person==contact&&location==place&&state(nowDay,minute)==READY;}
    static MeetupPlan find(int contact,int affinity,int day,int minute,Schedule schedule,IntUnaryOperator lot,IntUnaryOperator travel){
        if(!Household.validKey(contact)||affinity<MIN_AFFINITY||day<0||day>=1_000_000||minute<0||minute>=1440||schedule==null||lot==null||travel==null)return null;
        HashMap<Integer,Integer> routes=new HashMap<>();
        for(int delta=0;delta<=1;delta++){
            int candidateDay=day+delta;
            int first=delta==0?Math.max(420,((minute+14)/15)*15):420;
            for(int t=first;t<=1440-ARRIVAL-DURATION-1;t+=15){
                ResidentSchedule.Entry e=schedule.at(candidateDay,t);
                if(e==null||!Household.validKey(e.location)||!available(e,contact,e.location)||!publicLot(lot.applyAsInt(e.location)))continue;
                int cost=routes.computeIfAbsent(e.location,travel::applyAsInt);
                if(cost<0||cost>delta*1440+t-minute)continue;
                boolean free=true;
                for(int m=t+1;m<=t+ARRIVAL+DURATION;m++)if(!available(schedule.at(candidateDay,m),contact,e.location)){free=false;break;}
                if(free)return new MeetupPlan(contact,e.location,candidateDay,t);
            }
        }
        return null;
    }
    String save(int seed){return "1,"+seed+","+contact+","+place+","+day+","+start;}
    static MeetupPlan load(String data,int seed,int day,int minute,IntUnaryOperator affinity,IntUnaryOperator lot){
        if(data==null||data.length()>128||day<0||day>1_000_000||minute<0||minute>=1440||affinity==null||lot==null)return null;
        String[] v=data.split(",",-1);if(v.length!=6||!v[0].equals("1"))return null;
        try{
            int contact=Integer.parseInt(v[2]),place=Integer.parseInt(v[3]),d=Integer.parseInt(v[4]),t=Integer.parseInt(v[5]);
            if(Integer.parseInt(v[1])!=seed||!Household.validKey(contact)||!Household.validKey(place)||d<day||d>day+1||t<420||t+ARRIVAL+DURATION>=1440||t%15!=0||affinity.applyAsInt(contact)<MIN_AFFINITY||!publicLot(lot.applyAsInt(place)))return null;
            MeetupPlan p=new MeetupPlan(contact,place,d,t);return p.state(day,minute)==MISSED?null:p;
        }catch(NumberFormatException ignored){return null;}
    }
}
