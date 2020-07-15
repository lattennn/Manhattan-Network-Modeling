message l4Msg{
    int source; // Message source.
    int destination; // Destination (for random routing).
    int destination_column; // Column index of message destination.
    int destination_row; // Row index of message destination.
    int SEQ; // Sequence number.
    int hopcount = 0; // Hopcount.
    simtime_t ct = 0; // Creation time of first instance of the message.
    int id = 0; // Id of message.
}