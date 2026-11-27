package com.bore2.game;
public final class HouseholdTest {
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static int key(int x,int y){return x|(y<<16);}
    public static void main(String[] args){
        int home=key(500,424),friend=key(501,425),other=key(500,426);
        Household h=new Household();
        check(!h.invite(friend,home,29,0,true,true),"acquaintance declines");
        check(!h.invite(friend,home,30,1,true,true),"must settle overdue rent");
        check(!h.invite(friend,home,30,0,false,true),"real residence required");
        check(!h.invite(friend,home,30,0,true,false),"must be available");
        check(!h.invite(home,home,100,0,true,true),"no self or displaced resident");
        check(!h.invite(key(800,800),home,100,0,true,true),"local household only");
        check(h.invite(friend,home,30,0,true,true),"trusted available local friend moves in");
        check(!h.invite(other,home,100,0,true,true),"one roommate maximum");
        check(h.member()==friend&&h.residence()==home,"stable identity and address");
        check(h.dailyRent(home)==8&&h.dailyRent(other)==12,"contribution only for shared lease");
        int[] paid=h.rent(20,0,3,home);
        check(paid[0]==0&&paid[1]==4,"three days' shared rent, debt preserved");
        paid=h.rent(25,7,2,home);check(paid[0]==2&&paid[1]==0,"settle debt once");
        paid=h.rent(5,0,Integer.MAX_VALUE,home);check(paid[0]==0&&paid[1]==999995,"bounded billing");
        check(h.rent(20,0,5,-1)[0]==20,"no lease no daily rent");
        String saved=h.save(1234);Household loaded=new Household();
        loaded.load(saved,1234,home,30,true);check(loaded.member()==friend&&loaded.dailyRent(home)==8,"saved household restored");
        loaded.load(saved,1235,home,30,true);check(loaded.member()==-1,"world change invalidates membership");
        loaded.load(saved,1234,other,30,true);check(loaded.member()==-1,"address change invalidates membership");
        loaded.load(saved,1234,home,0,true);check(loaded.member()==-1,"unknown contact invalid");
        loaded.load(saved,1234,home,30,false);check(loaded.member()==-1,"nonresidential origin invalid");
        loaded.load("1,1234,garbage",1234,home,30,true);check(loaded.member()==-1,"corrupt save safe");
        loaded.load(saved,1234,home,30,true);loaded.clear();check(loaded.member()==-1&&loaded.dailyRent(home)==12,"move out restores rent");
        System.out.println("Household: eligibility, capacity, identity, bounded rent, saved residence and move-out PASS");
    }
}
