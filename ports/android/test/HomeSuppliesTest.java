package com.bore2.game;

public final class HomeSuppliesTest {
    private static void check(boolean b,String why){if(!b)throw new AssertionError(why);}
    public static void main(String[] args){
        check(HomeSupplies.sharedWindow(600,m->true),"whole meal window available");
        check(!HomeSupplies.sharedWindow(600,m->m!=615),"departures during meal are rejected");
        check(!HomeSupplies.sharedWindow(1430,m->true),"no crossing midnight without schedule check");
        HomeSupplies s=new HomeSupplies();
        check(s.cook(10,false)==5&&s.stock()==0,"legacy cooking still costs five without groceries");
        check(s.buy(7,HomeSupplies.GROCERIES,100,600,true)==-1,"restaurant cannot sell household stock");
        check(s.buy(6,HomeSupplies.GROCERIES,11,600,true)==-1&&s.stock()==0,"failed purchase is atomic");
        check(s.buy(6,HomeSupplies.GROCERIES,100,1260,true)==-1,"closed shop");
        check(s.buy(6,HomeSupplies.GROCERIES,100,600,true)==88&&s.stock()==3,"three ingredients cost twelve");
        check(s.cook(88,false)==88&&s.stock()==2,"cooking uses purchased groceries before cash");
        check(s.cook(88,true)==88&&s.stock()==0,"two servings consume two portions");
        s.load(1,0);check(s.cook(4,true)==-1&&s.stock()==1,"cannot partially consume ingredients on failed shared meal");
        check(s.cook(5,true)==0&&s.stock()==0,"one missing ingredient costs five");
        s.load(23,0);check(s.buy(31,HomeSupplies.GROCERIES,100,120,true)==-1&&s.stock()==23,"stock cap is atomic");
        s.load(21,0);check(s.buy(31,HomeSupplies.GROCERIES,100,120,true)==88&&s.stock()==24,"convenience groceries available at night");
        check(s.buy(31,HomeSupplies.MATTRESS,100,600,true)==-1,"furniture only from shop");
        check(s.buy(6,HomeSupplies.MATTRESS,100,600,false)==-1,"must lease home before furnishing");
        check(s.sleepMinutes()==480&&s.cookFood()==45&&s.cookSkill()==12&&s.restEnergy()==20,"base furniture behavior");
        int funds=s.buy(6,HomeSupplies.MATTRESS,100,600,true);check(funds==70&&s.sleepMinutes()==420,"mattress shortens sleep");
        check(s.buy(6,HomeSupplies.MATTRESS,funds,600,true)==-1,"cannot pay twice for same upgrade");
        funds=s.buy(6,HomeSupplies.STOVE,funds,600,true);check(funds==30&&s.cookFood()==55&&s.cookSkill()==16,"stove improves actual cooking");
        funds=s.buy(6,HomeSupplies.SOFA,funds,600,true);check(funds==5&&s.restEnergy()==30,"sofa increases rest recovery");
        HomeSupplies restored=new HomeSupplies();restored.load(s.stock(),s.upgrades());
        check(restored.stock()==24&&restored.sleepMinutes()==420&&restored.restEnergy()==30,"save round trip");
        restored.moveHome();check(restored.upgrades()==0&&restored.stock()==24,"furniture stays at old lease, groceries travel");
        restored.load(-100,999);check(restored.stock()==0&&restored.upgrades()==0,"invalid save falls back");
        restored.clear();check(restored.stock()==0&&restored.upgrades()==0,"new world reset");
        System.out.println("Home supplies: shop eligibility, atomic inventory, meal portions, functional upgrades, save and move-out PASS");
    }
}
