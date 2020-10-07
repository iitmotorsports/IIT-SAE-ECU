package main.java.javacom;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;

public class App {

    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();

    private static HashMap<Integer, byte[]> Teensy_Data = new HashMap<Integer, byte[]>();

    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }

    public static int getUnsignedShort(ByteBuffer bb) {
        return (bb.getShort() & 0xffff);
    }

    public static int getUnsignedShort(ByteBuffer bb, int position) {
        return (bb.getShort(position) & 0xffff);
    }

    public static long getUnsignedInt(ByteBuffer bb) {
        return ((long) bb.getInt() & 0xffffffffL);
    }

    public static long getUnsignedInt(ByteBuffer bb, int position) {
        return ((long) bb.getInt(position) & 0xffffffffL);
    }

    public enum TMSG {
        SPEED(258);

        public final int address;

        TMSG(int address) {
            this.address = address;
        }
    }

    public static int getSpeed() {
        byte[] data = Teensy_Data.get(TMSG.SPEED.address);
        if (data != null) {
            return (int) getUnsignedShort(ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN));
        } else {
            System.out.println("Value not received");
            return -1;
        }
    }

    public static long getAnotherValueThatExistsOnThisSpecificTeensyMsg() {
        byte[] data = Teensy_Data.get(TMSG.SPEED.address);
        if (data != null) {
            return getUnsignedInt(ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN), 3);
        } else {
            System.out.println("Value not received");
            return -1;
        }
    }

    /**
     * Get the ID from the byte array received from the teensy
     * 
     * @param data
     * @return Long
     */
    private static Integer getDataID(byte[] data) { // 0xDEAD => 57005
        byte[] add_chnk = Arrays.copyOfRange(data, 0, 2);
        return (int) getUnsignedShort(ByteBuffer.wrap(add_chnk).order(ByteOrder.LITTLE_ENDIAN));
    }

    /**
     * Get the data array from the byte array received from the teensy
     * 
     * @param data
     * @return byte[]
     */
    private static byte[] getDataVal(byte[] data) { // IMPROVE: Should it return a ByteBuffer instead?
        byte[] data_chnk = Arrays.copyOfRange(data, 2, data.length);
        return data_chnk;
    }

    private static void test(String portName) {
        PortConnect rxtx = new PortConnect();

        try {
            rxtx.connect(portName);

            byte[] data;
            while (true) {
                data = rxtx.read(); // IMPROVE: Check that data actually matches format
                // All the data that the teensy sends should be in bytes either way
                if (data != null) {
                    System.out.print(bytesToHex(data).replaceAll("..(?!$)", "$0|"));
                    // the real-time when data arrived
                    System.out.println((new SimpleDateFormat(" {HH:mm:ss}")).format(new Date()));

                    Teensy_Data.put(getDataID(data), getDataVal(data));
                    System.out.println(getSpeed());
                    System.out.println(getAnotherValueThatExistsOnThisSpecificTeensyMsg());
                    System.out.println(Teensy_Data);
                }
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            rxtx.close();
            System.out.println("Finished!");
        }
    }

    public static void main(String[] args) {
        test("COM6");
    }
}