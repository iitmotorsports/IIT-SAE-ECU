package main.java.javacom;

public class App {

    private static void test(String portName) {
        PortConnect rxtx = new PortConnect();

        try {
            rxtx.connect(portName);

            byte[] data;
            while (true) {
                data = rxtx.read(); // IMPROVE: Check that data actually matches format
                // All the data that the teensy sends should be in bytes either way
                if (data != null) {
                    TeensyMsg.setData(data);

                    System.out.println("Speed: " + TeensyMsg.ADD.SPEED.getValue());
                    System.out.println("Someother Val: " + TeensyMsg.ADD.ANOTHERVAL.getValue());
                    System.out.println(TeensyMsg.dataString());
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