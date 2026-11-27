package com.bore2.game;

/** Grocery portions and installed furniture upgrades. Failed transactions never mutate stock. */
final class HomeSupplies {
    static final int GROCERIES=0,MATTRESS=1,STOVE=2,SOFA=3;
    private static final int[] PRICE={12,30,40,25};
    private int stock,upgrades;
    int stock(){return stock;}
    int upgrades(){return upgrades;}
    boolean owned(int item){return item>=MATTRESS&&item<=SOFA&&(upgrades&(1<<(item-1)))!=0;}
    static boolean shop(int lot){return lot==6||lot==31;}
    static boolean sells(int lot,int item){return shop(lot)&&item>=GROCERIES&&item<=SOFA&&(item==GROCERIES||lot==6);}
    static int price(int item){return item>=0&&item<PRICE.length?PRICE[item]:-1;}
    static boolean open(int lot,int minute){return shop(lot)&&minute>=0&&minute<1440&&(lot==31||(minute>=420&&minute+15<=1260));}
    int buy(int lot,int item,int funds,int minute,boolean leased){
        if(!sells(lot,item)||!open(lot,minute)||funds<price(item))return -1;
        if(item==GROCERIES){if(stock>21)return -1;stock+=3;}
        else {if(!leased||owned(item))return -1;upgrades|=1<<(item-1);}
        return funds-price(item);
    }
    static boolean sharedWindow(int minute,java.util.function.IntPredicate available){
        if(minute<0||minute+30>=1440||available==null)return false;
        for(int t=minute;t<=minute+30;t++)if(!available.test(t))return false;
        return true;
    }
    int cook(int funds,boolean shared){
        int servings=shared?2:1,used=Math.min(stock,servings),cost=(servings-used)*5;
        if(funds<cost)return -1;
        stock-=used;return funds-cost;
    }
    int sleepMinutes(){return owned(MATTRESS)?420:480;}
    int cookFood(){return owned(STOVE)?55:45;}
    int cookSkill(){return owned(STOVE)?16:12;}
    int restEnergy(){return owned(SOFA)?30:20;}
    void load(int savedStock,int savedUpgrades){stock=savedStock>=0&&savedStock<=24?savedStock:0;upgrades=savedUpgrades>=0&&savedUpgrades<=7?savedUpgrades:0;}
    void moveHome(){upgrades=0;}
    void clear(){stock=upgrades=0;}
}
