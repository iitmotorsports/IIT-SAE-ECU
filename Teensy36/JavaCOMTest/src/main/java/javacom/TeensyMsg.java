package main.java.javacom;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class TeensyMsg {

    private static final byte[] HEX_ARRAY = "0123456789ABCDEF".getBytes(StandardCharsets.US_ASCII);
    private static final int ID_SIZE = 2; // How big is the teensy message ID
    private static HashMap<Integer, byte[]> Teensy_Data = new HashMap<Integer, byte[]>();

    /**
     * Enumurate the teensy addresses and define functions for each one that needs
     * a value exposed
     * 
     */
    public enum ADD {
        SPEED(258) {
            public long getValue() {
                return getUnsignedShort(address);
            }
        },
        ANOTHERVAL(258) {
            public long getValue() {
                return getUnsignedInt(address, 3);
            }
        };

        public final int address;

        abstract public long getValue(); // Changed to return 'Number' to return Integer or Long

        ADD(int address) {
            this.address = address;
        }
    }

    /**
     * Returns the hex string representation of the given byte array
     * 
     * @param bytes
     * @return The hex string
     */
    public static String hexStr(byte[] bytes) {
        byte[] hexChars = new byte[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars, StandardCharsets.UTF_8);
    }

    /**
     * Returns the hex string representation of the stored teensy byte array
     * 
     * @param bytes
     * @return The hex string
     */
    public static String msgHex(int MsgID) {
        byte[] bytes = Teensy_Data.get(MsgID);
        if (bytes == null)
            return "";
        return hexStr(bytes);
    }

    /**
     * Reads the first two bytes of a message's array, composing them into an
     * unsigned short value
     * 
     * @param data
     * @return The unsigned short value as an int
     */
    public static int getUnsignedShort(int MsgID) {
        byte[] data = Teensy_Data.get(MsgID);
        if (data == null)
            return -1;
        return (ByteBuffer.wrap(data).getShort(ID_SIZE) & 0xffff); // offset to ignore ID bytes
    }

    /**
     * Reads two bytes at the message's index, composing them into an unsigned short
     * value.
     * 
     * @param data
     * @param position
     * @return The unsigned short value as an int
     */
    public static int getUnsignedShort(int MsgID, int position) {
        byte[] data = Teensy_Data.get(MsgID);
        if (data == null)
            return -1;
        return (ByteBuffer.wrap(data).getShort(ID_SIZE + position) & 0xffff);
    }

    /**
     * Reads the first four bytes of a message's array, composing them into an
     * unsigned int value
     * 
     * @param data
     * @return The unsigned int value as a long
     */
    public static long getUnsignedInt(int MsgID) {
        byte[] data = Teensy_Data.get(MsgID);
        if (data == null)
            return -1;
        return ((long) ByteBuffer.wrap(data).getInt(ID_SIZE) & 0xffffffffL);
    }

    /**
     * Reads four bytes at the message's index, composing them into an unsigned int
     * value.
     * 
     * @param data
     * @param position
     * @return The unsigned short value at the buffer's current position as a long
     */
    public static long getUnsignedInt(int MsgID, int position) {
        byte[] data = Teensy_Data.get(MsgID);
        if (data == null)
            return -1;
        return ((long) ByteBuffer.wrap(data).getInt(ID_SIZE + position) & 0xffffffffL);
    }

    /**
     * Get the ID from the byte array received from the teensy
     * 
     * @param data
     * @return The message ID
     */
    private static int getDataID(byte[] raw_data) { // The ID 0xDEAD is 57005
        return ByteBuffer.wrap(raw_data).order(ByteOrder.LITTLE_ENDIAN).getShort() & 0xffff; // get TMsg ID
    }

    /**
     * Set teensy data in a HashMap given a raw byte array
     * 
     * @param raw_data
     */
    public static void setData(byte[] raw_data) {
        if (raw_data.length == 10) // 2 for ID, 8 for data bytes
            // Instead of chopping the original array up, just store the whole thing
            Teensy_Data.put(getDataID(raw_data), raw_data); // IMPROVE: check that ID is within range
    }

    /**
     * @return The string representation of the stored teensy messages
     */
    public static String dataString() {
        StringBuilder str = new StringBuilder();
        for (Map.Entry<Integer, byte[]> e : Teensy_Data.entrySet()) {
            str.append(e.getKey());
            str.append(" : ");
            str.append(hexStr(e.getValue()).replaceAll("..(?!$)", "$0|"));
            str.append('\n');
        }
        return str.toString();
    }
}