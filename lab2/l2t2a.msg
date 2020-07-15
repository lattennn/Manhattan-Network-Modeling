message l2t2aMsg{
    int source; // Message source
    int seq; // Sequence number.
    int destination_column; // Destination column
    int destination_row; // Destination row
    int delivery_delay = 0; // Delivery delay.
}
