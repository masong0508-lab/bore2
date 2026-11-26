package com.bore2.game;

final class NativeWorld {
    static { System.loadLibrary("bore2_world"); }
    static synchronized native int init(int seed);
    static synchronized native int[] tiles(int x, int y, int radius);
    static synchronized native String describe(int x, int y, int weekday);
    static synchronized native float[] mesh(int x, int y, int radius);
    static synchronized native boolean canVisit(int x,int y);
    static synchronized native int[] activity(int x,int y,int day,int slot,int minute,int funds,int rest);
    static synchronized native String activityText(int id,int field);
    private NativeWorld() {}
}
