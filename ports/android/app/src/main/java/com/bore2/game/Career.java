package com.bore2.game;

import java.util.function.BiPredicate;

/** A regular employer is tied to the generated address; casual work remains possible. */
final class Career {
    private static final int LIMIT=1_000_000;
    private static final int[] PROMOTION={3,7,12,18};
    private static final String[] RANK={"Trainee","Associate","Experienced","Senior","Lead"};
    private int employer=-1,lot=-1,shifts,lastDay=-1;
    int employer(){return employer;}
    int lot(){return lot;}
    int shifts(){return shifts;}
    int lastDay(){return lastDay;}
    boolean employed(){return employer>=0;}
    static boolean validKey(int key){return key>=0&&(key&65535)<1024&&(key>>>16)<1024;}
    static boolean workLot(int lot){switch(lot){case 6:case 7:case 8:case 9:case 14:case 20:case 21:case 25:case 27:case 29:case 30:case 31:return true;default:return false;}}
    boolean matches(int key,int type){return employed()&&key==employer&&type==lot;}
    boolean hire(int key,int type){
        if(employed()||!validKey(key)||!workLot(type))return false;
        employer=key;lot=type;shifts=0;return true;
    }
    void quit(){employer=lot=-1;shifts=0;}
    void reset(){quit();lastDay=-1;}
    int level(){int n=0;while(n<PROMOTION.length&&shifts>=PROMOTION[n])n++;return n;}
    String rank(){return RANK[level()];}
    int bonus(){return employed()?level()*10:0;}
    int nextPromotion(){int level=level();return level<PROMOTION.length?PROMOTION[level]-shifts:0;}
    boolean canWork(int day){return day>=0&&day<=LIMIT&&day>lastDay;}
    int work(int key,int type,int day,int basePay,int experience){
        if(!validKey(key)||!workLot(type)||!canWork(day)||basePay<=0||basePay>1000)return -1;
        int pay=basePay+Math.min(5,Math.max(0,experience)/4)*10;
        if(matches(key,type)){pay+=bonus();shifts=Math.min(LIMIT,shifts+1);}
        lastDay=day;return pay;
    }
    String save(int seed){return employed()?"1,"+seed+","+employer+","+lot+","+shifts:"";}
    void load(String data,int seed,int day,int savedLastDay,BiPredicate<Integer,Integer> siteValid){
        reset();lastDay=Math.max(-1,Math.min(Math.max(0,day),savedLastDay));
        if(data==null||data.length()>100||siteValid==null)return;
        String[] v=data.split(",",-1);if(v.length!=5||!v[0].equals("1"))return;
        try{
            int key=Integer.parseInt(v[2]),type=Integer.parseInt(v[3]),count=Integer.parseInt(v[4]);
            if(Integer.parseInt(v[1])!=seed||count<0||count>LIMIT||!validKey(key)||!workLot(type)||!siteValid.test(key,type))return;
            employer=key;lot=type;shifts=count;
        }catch(NumberFormatException ignored){/* Old or damaged record: no active job. */}
    }
}
