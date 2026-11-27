package com.bore2.game;

import java.util.Arrays;
public final class LifeRulesTest {
    static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    public static void main(String[] args){
        check(LifeRules.isHome(1)&&LifeRules.isHome(5)&&!LifeRules.isHome(6),"residential lots only");
        check(LifeRules.isFood(7)&&LifeRules.isFood(21)&&LifeRules.isFood(31)&&!LifeRules.isFood(17),"food venue classification");
        check(LifeRules.isWork(9)&&LifeRules.isWork(20)&&!LifeRules.isWork(17),"work matches native shift rules");
        int[] path={LifeRules.pack(10,11),LifeRules.pack(11,11),LifeRules.pack(12,11)};
        check(LifeRules.validRoute(10,10,12,11,path),"cardinal route");
        check(!LifeRules.validRoute(10,10,12,11,new int[]{path[2]}),"reject teleport");
        check(!LifeRules.validRoute(10,10,12,12,path),"reject wrong destination");
        check(!LifeRules.validRoute(10,10,12,11,null),"reject null route");
        check(!LifeRules.validRoute(0,0,0,0,new int[]{LifeRules.pack(1024,0)}),"reject off-map route");
        check(LifeRules.travelEnergy(1)==1&&LifeRules.travelEnergy(4)==1&&LifeRules.travelEnergy(5)==2,"travel energy");
        check(LifeRules.travelMinutes(12)==36,"travel time");
        check(Arrays.equals(LifeRules.rent(20,0,3,true),new int[]{0,16}),"unpaid rent carried");
        check(Arrays.equals(LifeRules.rent(30,16,0,true),new int[]{14,0}),"rent debt paid once");
        check(Arrays.equals(LifeRules.rent(20,0,100,false),new int[]{20,0}),"no rent without home");
        check(Arrays.equals(LifeRules.rent(20,0,Integer.MAX_VALUE,true),new int[]{0,999980}),"bounded rent arithmetic");
        check(LifeRules.elapsedBuckets(1439,2,120)==1&&LifeRules.elapsedBuckets(119,1,120)==1,"fractional time decay");
        check(LifeRules.credit(999999,80)==1000000,"bounded funds");
        check(LifeRules.payBonus(0)==0&&LifeRules.payBonus(4)==10&&LifeRules.payBonus(10000)==50,"career progression cap");
        int[] tiles=new int[25];Arrays.fill(tiles,2);tiles[12]=2|(6<<3);tiles[11]=2|(1<<3);tiles[13]=2|(5<<3);tiles[7]=(1<<3);tiles[0]=2|(2<<3);
        int[] found=LifeRules.nearby(tiles,10,10,2,10,10,LifeRules.HOMES,3);
        check(Arrays.equals(found,new int[]{LifeRules.pack(9,10),LifeRules.pack(11,10),LifeRules.pack(8,8)}),"nearest homes, no water, deterministic ties");
        check(LifeRules.nearby(new int[0],10,10,2,10,10,LifeRules.HOMES,3).length==0,"incomplete map");
        System.out.println("Android life rules PASS");
    }
}
