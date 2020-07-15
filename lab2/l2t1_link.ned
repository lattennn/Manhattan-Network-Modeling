simple l2t1_link
{
    parameters:
        @display("i=block/routing");
        double L @unit(ms); // Parameter for variable delay, accessible to modules.
    gates:
        input in[]; // Connections incoming from other modules.
        input ftx; // Connection coming from tx of same module.
        output out[];// Connections going to other modules.
        output trx; // Connection going towards rx of same module.
}