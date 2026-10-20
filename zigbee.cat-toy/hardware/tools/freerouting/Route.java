// Headless batch autorouter for Freerouting 1.3.x (the Fedora flatpak build has no CLI batch mode).
// Usage: java -Djava.awt.headless=true -cp freerouting.jar Route.java in.dsn out.ses [passes]
// The GUI panel is allocated without running its constructor; only the routing core is used.
import java.io.*;
import java.lang.reflect.*;
import java.util.Locale;

public class Route {
    static class Worker extends interactive.InteractiveActionThread {
        Worker(interactive.BoardHandling h) { super(h); }
        @Override protected void thread_action() {}
    }

    /** Panel with all GUI callbacks stubbed out. Instantiated via Unsafe, so the ctor never runs. */
    static class Panel extends gui.BoardPanel {
        Panel() { super(null, null, null, false); }
        @Override public void set_selected_signal_layer(int l) {}
        @Override public void paintComponent(java.awt.Graphics g) {}
        @Override public void repaint() {}
        @Override public void repaint(long t, int x, int y, int w, int h) {}
        @Override public void repaint(java.awt.Rectangle r) {}
        @Override public void move_mouse(java.awt.geom.Point2D p) {}
        @Override public void center_display(java.awt.geom.Point2D p) {}
        @Override public java.awt.geom.Point2D zoom(double f, java.awt.geom.Point2D p) { return p; }
        @Override public void zoom_with_mouse_wheel(java.awt.geom.Point2D p, int r) {}
        @Override public void zoom_in(java.awt.geom.Point2D p) {}
        @Override public void zoom_out(java.awt.geom.Point2D p) {}
        @Override public void zoom_frame(java.awt.geom.Point2D a, java.awt.geom.Point2D b) {}
        @Override public java.awt.geom.Point2D get_viewport_center() { return new java.awt.geom.Point2D.Double(); }
        @Override public java.awt.Point get_viewport_position() { return new java.awt.Point(); }
        @Override public void set_custom_crosshair_cursor(boolean b) {}
        @Override public java.awt.Dimension getSize() { return new java.awt.Dimension(1000, 1000); }
    }

    static Thread watchdog(Worker w, board.RoutingBoard rb, int secs, String phase) {
        Thread t = new Thread(() -> {
            long t0 = System.currentTimeMillis();
            try {
                while (true) {
                    Thread.sleep(10000);
                    long el = (System.currentTimeMillis() - t0) / 1000;
                    System.out.println("  [" + phase + " " + el + " s] traces=" + rb.get_traces().size()
                            + " vias=" + rb.get_vias().size());
                    if (el >= secs) { System.out.println("  time limit, stopping " + phase); w.request_stop(); return; }
                }
            } catch (InterruptedException e) { /* phase finished */ }
        });
        t.setDaemon(true);
        t.start();
        return t;
    }

    static Object allocate(Class<?> c) throws Exception {
        Field f = Class.forName("sun.misc.Unsafe").getDeclaredField("theUnsafe");
        f.setAccessible(true);
        Object unsafe = f.get(null);
        return unsafe.getClass().getMethod("allocateInstance", Class.class).invoke(unsafe, c);
    }

    static void setField(Object o, Class<?> c, String name, Object value) throws Exception {
        Field f = c.getDeclaredField(name);
        f.setAccessible(true);
        f.set(o, value);
    }

    public static void main(String[] args) throws Exception {
        String in = args[0], out = args[1];
        int passes = args.length > 2 ? Integer.parseInt(args[2]) : 99;
        javax.swing.JLabel l1 = new javax.swing.JLabel(), l2 = new javax.swing.JLabel(),
                l3 = new javax.swing.JLabel(), l4 = new javax.swing.JLabel();
        interactive.ScreenMessages msgs = new interactive.ScreenMessages(l1, l2, l3, l4, Locale.ENGLISH);
        gui.BoardPanel panel = (gui.BoardPanel) allocate(Panel.class);
        setField(panel, gui.BoardPanel.class, "screen_messages", msgs);
        // BoardHandling's constructor wires up GUI menus/toolbars; skip it and set the core fields.
        interactive.BoardHandling h = (interactive.BoardHandling) allocate(interactive.BoardHandling.class);
        Class<?> bh = interactive.BoardHandling.class;
        setField(h, bh, "panel", panel);
        setField(h, bh, "locale", Locale.ENGLISH);
        setField(h, bh, "screen_messages", msgs);
        setField(h, bh, "logfile", new interactive.Logfile());
        setField(h, bh, "resources", java.util.ResourceBundle.getBundle("interactive.resources.BoardHandling", Locale.ENGLISH));
        setField(h, bh, "current_mouse_position", new geometry.planar.FloatPoint(0, 0));
        setField(panel, gui.BoardPanel.class, "board_handling", h);

        try (InputStream is = new FileInputStream(in)) {
            Object r = h.import_design(is, new board.BoardObserverAdaptor(), new board.ItemIdNoGenerator(),
                    board.TestLevel.RELEASE_VERSION);
            System.out.println("import: " + r);
        }
        board.RoutingBoard rb = h.get_routing_board();
        rb.rules.set_trace_angle_restriction(board.AngleRestriction.FORTYFIVE_DEGREE);   // 45-degree only
        System.out.println("layers: " + rb.get_layer_count() + "  items: " + rb.get_items().size());

        interactive.AutorouteSettings s = h.settings.autoroute_settings;
        int routeSecs = args.length > 3 ? Integer.parseInt(args[3]) : 300;
        int optSecs = args.length > 4 ? Integer.parseInt(args[4]) : 120;
        if (args.length > 5) s.set_start_ripup_costs(Integer.parseInt(args[5]));
        System.out.println("incomplete before: " + new interactive.RatsNest(rb, Locale.ENGLISH).incomplete_count());

        Worker w = new Worker(h);
        Thread dog = watchdog(w, rb, routeSecs, "route");
        long t0 = System.currentTimeMillis();
        boolean done = new autoroute.BatchAutorouter(w, false, true, s.get_start_ripup_costs()).autoroute_passes();
        dog.interrupt();
        System.out.println("ripup " + s.get_start_ripup_costs() + ": autoroute complete=" + done + " in " + (System.currentTimeMillis() - t0) / 1000 + " s, incomplete "
                + new interactive.RatsNest(rb, Locale.ENGLISH).incomplete_count());

        Worker w2 = new Worker(h);
        Thread dog2 = watchdog(w2, rb, optSecs, "optimize");
        long t1 = System.currentTimeMillis();
        new autoroute.BatchOptRoute(w2).optimize_board();
        dog2.interrupt();
        System.out.println("optimize " + (System.currentTimeMillis() - t1) / 1000 + " s, incomplete "
                + new interactive.RatsNest(rb, Locale.ENGLISH).incomplete_count());

        try (OutputStream os = new FileOutputStream(out)) {
            boolean ok = h.export_specctra_session_file(new File(in).getName(), os);
            System.out.println("ses written: " + ok);
        }
        System.exit(0);
    }
}
