// runme.java

import AIOUSB.*;

public class test {
 static {
    System.loadLibrary("AIOUSB");
  }

  public static void main(String argv[]) {
    System.out.println(AIOUSB.AIOUSB_Init());
    AIOUSB.AIOUSB_ListDevices();
  }
}
