package com.bore2.game;

/** Immutable, compact appearance shared by outdoor and interior rendering. */
final class AvatarStyle {
    private static final int TAG=0x100000;
    static final String[] FIELDS={"Skin tone","Hair color","Top","Trousers","Hair style"};
    private static final String[][] OPTIONS={
        {"Porcelain","Light","Warm","Olive","Brown","Deep"},
        {"Black","Brown","Chestnut","Blond","Silver","Copper"},
        {"Teal","Blue","Red","Olive","Cream","Purple","Gold","Charcoal"},
        {"Slate","Denim","Khaki","Black","Rust","Cream"},
        {"Short","Swept","Bob","Close cut"}};
    private static final int[][] COLORS={
        {0xf2d4bd,0xe4bd9d,0xd5a17b,0xb48d64,0x8d5c42,0x593e33},
        {0x25232a,0x4e342a,0x824e34,0xc8a76a,0xaab0b4,0xb15a35},
        {0x348d87,0x477ca9,0xb34b51,0x758752,0xd3c7ae,0x8266a1,0xc49a42,0x41474e},
        {0x354757,0x486789,0xa38d68,0x33333d,0x925947,0xbcb29d}};
    private final int[] values;
    private AvatarStyle(int[] values){this.values=values;}
    static AvatarStyle defaults(){return new AvatarStyle(new int[]{2,1,0,0,0});}
    static String[] options(int field){return field>=0&&field<OPTIONS.length?OPTIONS[field].clone():new String[0];}
    int choice(int field){return field>=0&&field<values.length?values[field]:-1;}
    int color(int field){return field>=0&&field<COLORS.length?COLORS[field][values[field]]:0x333333;}
    AvatarStyle with(int field,int value){
        if(field<0||field>=values.length||value<0||value>=OPTIONS[field].length)return this;
        int[] next=values.clone();next[field]=value;return new AvatarStyle(next);
    }
    int encode(){int p=TAG;for(int i=0;i<values.length;i++)p|=values[i]<<(i*3);return p;}
    static AvatarStyle decode(int p){
        if((p&~0x7fff)!=TAG)return defaults();
        int[] next=new int[5];for(int i=0;i<next.length;i++){next[i]=(p>>>(i*3))&7;if(next[i]>=OPTIONS[i].length)return defaults();}
        return new AvatarStyle(next);
    }
    private static int mix(int x){x^=x>>>16;x*=0x7feb352d;x^=x>>>15;x*=0x846ca68b;return x^(x>>>16);}
    static AvatarStyle resident(int seed,int home){
        int[] next=new int[5];for(int i=0;i<next.length;i++)next[i]=Math.floorMod(mix(seed^home^(0x9e3779b9*(i+1))),OPTIONS[i].length);
        return new AvatarStyle(next);
    }
    static String cleanName(String raw){
        if(raw==null)return "Bore";
        StringBuilder out=new StringBuilder();boolean space=false;int count=0;
        for(int i=0;i<raw.length()&&count<20;){int c=raw.codePointAt(i);i+=Character.charCount(c);
            if(Character.isWhitespace(c)||Character.isSpaceChar(c)){space=out.length()>0;continue;}
            if(Character.isISOControl(c)||Character.getType(c)==Character.FORMAT||Character.getType(c)==Character.SURROGATE)continue;
            if(space&&count<19){out.append(' ');count++;}space=false;
            if(count<20){out.appendCodePoint(c);count++;}
        }
        return out.length()==0?"Bore":out.toString();
    }
}
