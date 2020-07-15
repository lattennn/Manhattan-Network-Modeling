message l2t1Msg{
    int source; // Message source.
    int destination_column; // Column index of message destination.
    int destination_row; // Row index of message destination.
    int delivery_delay = 0; // Delivery delay, updated at every transmission.
}