// runme.java

import com.accesio.*;

public class test {
 static {
    System.loadLibrary("AIOUSB");
  }

  public static void main(String argv[]) {
    System.out.println(AIOUSB.AIOUSB_Init());
    AIOUSB.AIOUSB_ListDevices();
  }
}
