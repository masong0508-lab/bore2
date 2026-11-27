package com.bore2.game;

/** One consenting roommate, with identity separate from the shared address. */
final class Household {
    static final int REQUIRED_AFFINITY=30, CONTRIBUTION=4, FULL_RENT=12;
    private static final int LIMIT=1_000_000;
    private int member=-1,residence=-1;
    int member(){return member;}
    int residence(){return residence;}
    void clear(){member=residence=-1;}
    static boolean validKey(int key){return key>=0&&(key&65535)<1024&&(key>>>16)<1024;}
    static boolean local(int member,int home){
        return validKey(member)&&validKey(home)&&member!=home&&Math.abs((member&65535)-(home&65535))+Math.abs((member>>>16)-(home>>>16))<=3;
    }
    boolean invite(int key,int home,int affinity,int debt,boolean residential,boolean available){
        if(member>=0||!local(key,home)||affinity<REQUIRED_AFFINITY||debt!=0||!residential||!available)return false;
        member=key;residence=home;return true;
    }
    int dailyRent(int home){return member>=0&&residence==home?FULL_RENT-CONTRIBUTION:FULL_RENT;}
    int[] rent(int funds,int debt,int days,int home){
        long bill=Math.max(0,Math.min(LIMIT,(long)debt+(validKey(home)?(long)Math.max(0,days)*dailyRent(home):0)));
        int cash=Math.max(0,Math.min(LIMIT,funds)),paid=(int)Math.min(cash,bill);
        return new int[]{cash-paid,(int)bill-paid};
    }
    String save(int seed){return member<0?"":"1,"+seed+","+residence+","+member;}
    static int savedMember(String data,int seed,int home){
        if(data==null||data.length()>80||!validKey(home))return -1;
        String[] v=data.split(",",-1);if(v.length!=4||!v[0].equals("1"))return -1;
        try{int key=Integer.parseInt(v[3]);return Integer.parseInt(v[1])==seed&&Integer.parseInt(v[2])==home&&local(key,home)?key:-1;}
        catch(NumberFormatException ignored){return -1;}
    }
    void load(String data,int seed,int home,int affinity,boolean residential){
        clear();int key=savedMember(data,seed,home);
        if(key>=0&&affinity>=REQUIRED_AFFINITY&&residential){member=key;residence=home;}
    }
}
