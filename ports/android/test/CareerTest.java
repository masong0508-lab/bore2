package com.bore2.game;

public final class CareerTest {
    private static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    private static int key(int x,int y){return x|(y<<16);}
    public static void main(String[] args){
        int shop=key(500,423),office=key(501,423),park=key(502,423);
        Career c=new Career();
        check(!c.hire(park,16)&&!c.hire(-1,6),"only actual supported job types and valid addresses");
        check(c.work(shop,6,0,70,0)==70,"casual shifts still available");
        check(c.work(shop,6,0,70,1)==-1,"one shift per game day");
        check(c.hire(shop,6)&&!c.hire(office,9),"one regular employer at a time");
        check(c.work(shop,6,0,70,1)==-1,"hiring cannot bypass daily limit");
        check(c.work(office,9,1,100,0)==100&&c.shifts()==0,"casual work elsewhere does not promote employer");
        for(int d=2;d<=4;d++)check(c.work(shop,6,d,70,0)==70,"entry wages before first promotion");
        check(c.level()==1&&c.bonus()==10&&c.shifts()==3,"three shifts earn first promotion");
        check(c.work(shop,6,5,70,10)==100,"career and established experience bonuses");
        check(c.work(shop,9,6,100,0)==100&&c.shifts()==4,"different lot type does not count as same employer");
        check(c.work(shop,6,5,70,0)==-1,"clock rollback cannot pay twice");
        String saved=c.save(1234);Career restored=new Career();
        restored.load(saved,1234,6,6,(place,lot)->place==shop&&lot==6);
        check(restored.employer()==shop&&restored.lot()==6&&restored.shifts()==4&&restored.lastDay()==6,"career and daily shift restore");
        check(restored.work(shop,6,6,70,0)==-1,"reload cannot repeat work");
        restored.quit();check(!restored.employed()&&restored.lastDay()==6,"quit does not reset daily limit");
        check(restored.hire(office,9)&&restored.work(office,9,6,100,0)==-1,"switching employers cannot repeat shift");
        restored.load(saved,1235,6,6,(place,lot)->true);check(!restored.employed()&&restored.lastDay()==6,"wrong world drops job but not daily limit");
        restored.load(saved,1234,6,6,(place,lot)->false);check(!restored.employed(),"changed actual workplace drops job");
        restored.load("1,1234,32768500,6,999999999",1234,6,6,(place,lot)->true);check(!restored.employed(),"corrupt record rejected");
        restored.load("",1234,2,99,(place,lot)->true);check(restored.lastDay()==2&&!restored.canWork(2),"future saved day clamped safely");
        restored.load("",1234,5,4,(place,lot)->true);check(restored.canWork(5)&&restored.lastDay()==4,"old save keeps shift history");
        restored.reset();check(!restored.employed()&&restored.lastDay()==-1,"new world resets employment and history");
        check(restored.hire(shop,6),"new contract");
        for(int d=0;d<100;d++)check(restored.work(shop,6,d,70,10000)>0,"bounded progression");
        check(restored.level()==4&&restored.bonus()==40&&restored.shifts()==100,"five levels, capped salary bonus");
        check(restored.work(shop,6,100,70,1_000_000_000)==160,"extreme experience cannot overflow wage");
        System.out.println("Career: employer identity, casual shifts, promotions, daily pay guard, restore and quit PASS");
    }
}
