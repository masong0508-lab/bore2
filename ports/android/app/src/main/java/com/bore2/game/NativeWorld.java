package com.bore2.game;

final class NativeWorld {
    static { System.loadLibrary("bore2_world"); }
    static native int init(int seed);
    static native int[] tiles(int x, int y, int radius);
    static native String describe(int x, int y, int weekday);
    private NativeWorld() {}
}
