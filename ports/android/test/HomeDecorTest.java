package com.bore2.game;

import java.util.Arrays;

public final class HomeDecorTest {
    private static void check(boolean ok,String msg){if(!ok)throw new AssertionError(msg);}
    private static int cell(int x,int y){return y*18+x;}
    private static int[] layout(){
        int[] raw=new int[327];raw[0]=4;raw[1]=4;raw[2]=7;
        for(int y=3;y<=7;y++)for(int x=3;x<=13;x++)if(x!=8||y==5)raw[3+cell(x,y)]=1;
        return raw;
    }
    private static BuildingPlan home(){BuildingPlan b=BuildingPlan.create(layout(),123,2,false);check(b!=null,"fixture layout");return b;}
    public static void main(String[] args){
        int key=500|(424<<16);BuildingPlan b=home();InteriorPlan p=b.floor(0);
        HomeDecor d=HomeDecor.create(b,1234,key,"");check(d!=null,"own home can be decorated");
        int[] original=p.objects.clone();int player=p.entry;
        check(!d.move(0,0,p.entry,player),"cannot cover entrance");
        check(!d.move(0,0,b.landing,player),"cannot cover stairs");
        check(!d.move(0,0,p.objects[1],player),"cannot overlap another object");
        check(!d.move(0,0,0,player),"cannot place outside footprint");
        check(!d.move(0,0,cell(8,5),player),"cannot obstruct connecting passage");
        check(Arrays.equals(original,p.objects),"failed moves are atomic");
        int spot=d.nextSpot(0,0,player);check(spot>=0&&spot!=original[0],"find a valid new position");
        check(p.objects[0]==spot&&p.occupied[spot]==0&&p.occupied[original[0]]==-1,"update both object and collision data");
        for(int k=0;k<p.actionCount();k++)check(p.routeToObject(player,k)!=null,"all furniture stays usable");
        check(p.route(player,b.landing)!=null,"stairs stay reachable");
        check(!d.move(0,0,original[0],original[0]),"cannot cover player");
        check(d.setTheme(0,2)&&d.setTheme(1,4),"per-floor themes");
        check(!d.setTheme(0,999)&&d.theme(0)==2&&d.theme(1)==4,"invalid theme rejected");
        String saved=d.save();BuildingPlan fresh=home();HomeDecor restored=HomeDecor.create(fresh,1234,key,saved);
        check(restored.theme(0)==2&&restored.theme(1)==4&&fresh.floor(0).objects[0]==spot,"round trip changed layout and theme");
        check(Arrays.equals(b.floor(0).objects,fresh.floor(0).objects),"all saved objects restored");
        check(!restored.reset(0,original[0]),"reset cannot put furniture on player");
        check(restored.reset(0,fresh.floor(0).entry)&&Arrays.equals(original,fresh.floor(0).objects)&&restored.theme(0)==0&&restored.theme(1)==4,"reset only current floor");
        BuildingPlan other=home();HomeDecor wrong=HomeDecor.create(other,1234,key+1,saved);
        check(wrong.theme(0)==0&&Arrays.equals(original,other.floor(0).objects),"different home cannot inherit layout");
        other=home();wrong=HomeDecor.create(other,1235,key,saved);check(wrong.theme(0)==0&&Arrays.equals(original,other.floor(0).objects),"different seed cannot inherit layout");
        String[] fields=saved.split(";",-1);String[] floor=fields[1].split(",",-1);floor[1]=Integer.toString(player);fields[1]=String.join(",",floor);
        other=home();wrong=HomeDecor.create(other,1234,key,String.join(";",fields));
        check(wrong.theme(0)==0&&Arrays.equals(original,other.floor(0).objects)&&wrong.theme(1)==4,"corrupted floor rejected without destroying valid upper floor");
        other=home();wrong=HomeDecor.create(other,1234,key,"x".repeat(2049));check(wrong.theme(0)==0,"oversize save rejected");
        int[] venue=layout();venue[0]=14;check(HomeDecor.create(BuildingPlan.create(venue,123,2,true),1234,key,"")==null,"public buildings not editable");
        System.out.println("HomeDecor: safe relocation, passage/door/stair protection, room themes, save/reload, reset and corrupt save recovery PASS");
    }
}
