message l3Msg{
    int source; // Message source.
    int destination; // Destination (for random routing).
    int destination_column; // Column index of message destination.
    int destination_row; // Row index of message destination.
    int SEQ; // Sequence number.
    int delivery_delay = 0; // Delivery delay, updated at every transmission.
    int hopcount = 0; // Hopcount.
}