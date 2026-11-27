package com.bore2.game;


/** Saved home-only furniture and room palette changes, bound to a world and address. */
final class HomeDecor {
    static final String[] THEMES={"Honey oak","Coastal","Sage","Terracotta","Slate","Cream"};
    private static final int[][] COLORS={
        {0xffd8c19c,0xffd0b58d,0xff56473d,0xff82a6a3},
        {0xffc9d8d8,0xffb9cbcf,0xff4d6875,0xff5c9bac},
        {0xffccd3b5,0xffbbc7a5,0xff52664d,0xff809a6c},
        {0xffe4c0a5,0xffd7aa8d,0xff814f40,0xffb96e53},
        {0xffb8c1cc,0xffa9b5c1,0xff414d61,0xff7a8eac},
        {0xffeee0c7,0xffe1d1b6,0xff796650,0xffb9a385}};
    private final BuildingPlan building;
    private final int seed,key;
    private final int[][] defaults;
    private final int[] themes;
    private HomeDecor(BuildingPlan building,int seed,int key){
        this.building=building;this.seed=seed;this.key=key;
        defaults=new int[building.count()][];themes=new int[building.count()];
        for(int i=0;i<defaults.length;i++)defaults[i]=building.floor(i).objects.clone();
    }
    static HomeDecor create(BuildingPlan building,int seed,int key,String saved){
        if(building==null||building.count()<1||building.floor(0).commercial)return null;
        HomeDecor out=new HomeDecor(building,seed,key);out.restore(saved);return out;
    }
    int theme(int floor){return floor>=0&&floor<themes.length?themes[floor]:0;}
    static int color(int theme,int part){return COLORS[Math.max(0,Math.min(COLORS.length-1,theme))][part];}
    boolean setTheme(int floor,int theme){
        if(building.floor(floor)==null||theme<0||theme>=THEMES.length)return false;
        themes[floor]=theme;return true;
    }
    boolean move(int floor,int slot,int target,int player){
        InteriorPlan p=building.floor(floor);if(p==null||slot<0||slot>=p.actionCount()||!p.walkable(player))return false;
        if(target==p.objects[slot])return true;
        if(!p.walkable(target))return false;
        int[] next=p.objects.clone();next[slot]=target;
        return p.replaceObjects(next,building.landing,player);
    }
    int nextSpot(int floor,int slot,int player){
        InteriorPlan p=building.floor(floor);if(p==null||slot<0||slot>=p.actionCount())return -1;
        int start=p.objects[slot];
        for(int d=1;d<InteriorPlan.N;d++){
            int target=(start+d)%InteriorPlan.N;
            if(move(floor,slot,target,player))return target;
        }
        return -1;
    }
    boolean reset(int floor,int player){
        InteriorPlan p=building.floor(floor);
        if(p==null||!p.walkable(player)||!p.replaceObjects(defaults[floor],building.landing,player))return false;
        themes[floor]=0;return true;
    }
    String save(){
        StringBuilder s=new StringBuilder("1,").append(seed).append(',').append(key).append(',').append(building.count());
        for(int n=0;n<building.count();n++){
            s.append(';').append(themes[n]);for(int cell:building.floor(n).objects)s.append(',').append(cell);
        }
        return s.toString();
    }
    private void restore(String data){
        if(data==null||data.isEmpty()||data.length()>2048)return;
        String[] rows=data.split(";",-1);
        if(rows.length!=building.count()+1||!rows[0].equals("1,"+seed+","+key+","+building.count()))return;
        for(int n=0;n<building.count();n++){
            String[] v=rows[n+1].split(",",-1);if(v.length!=InteriorPlan.COUNT+1)continue;
            try{
                int theme=Integer.parseInt(v[0]);if(theme<0||theme>=THEMES.length)continue;
                int[] cells=new int[InteriorPlan.COUNT];for(int i=0;i<cells.length;i++)cells[i]=Integer.parseInt(v[i+1]);
                if(building.floor(n).replaceObjects(cells,building.landing,-1))themes[n]=theme;
            }catch(NumberFormatException ignored){/* Leave this floor at its generated defaults. */}
        }
    }
}
