#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "iostream"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return next_seqno_ - latest_ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  (void)transmit;

  // Edge cases
  if (fin_sent) {
    return;
  }

  TCPSenderMessage msg = TCPSenderMessage();

  // Handle SYN
  if (!syn_sent) {
    syn_sent = true;

    msg.SYN = true;
    if (reader().is_finished()) {
      msg.FIN = true;
      fin_sent = true;
    }

    send_segment(msg, transmit);
    return;
  }

  // Special case: Send 1 byte probes if reciever window was zero.
  // Helpful for probing window size
  uint16_t window_size = max((uint16_t)1, latest_window_size_);

  // Send segments until window is full or until stream has data
  while (sequence_numbers_in_flight() < window_size and !fin_sent)
  {
    // Remaining space in window
    uint64_t remain = window_size - sequence_numbers_in_flight();
    size_t max_payload_size = min(static_cast<size_t>(remain), TCPConfig::MAX_PAYLOAD_SIZE);

    // Read stream
    read(reader(), max_payload_size, msg.payload);

    // Add FIN iff stream is finished and window still has space
    if (reader().is_finished() and msg.sequence_length() < window_size) {
      msg.FIN = true;
      fin_sent = true;
    }

    // Ignore Empty Segment
    if (msg.sequence_length() == 0) {
      return;
    }

    send_segment(msg, transmit);
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg = TCPSenderMessage();
  msg.seqno = get_wrapped_next_seqno();
  msg.RST = reader().has_error();
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;

  // Edge cases
  if (msg.RST) {
    writer().set_error();
    return;
  }

  // ACK even before SYN is sent
  if (!msg.ackno.has_value()) {
    return;
  }
  
  uint64_t recv_ackno = msg.ackno.value().unwrap(isn_, next_seqno_);

  // Cannot recive ACK even before segment is sent
  if (recv_ackno > next_seqno_)
  {
    return;
  }

  latest_window_size_ = msg.window_size;

  // Duplicate ACK
  if (recv_ackno <= latest_ackno_)
  {
    return;
  }
  
  latest_ackno_ = recv_ackno;

  // Pop ACKed segments
  while (!q.empty() and q.front().seqno.unwrap(isn_, next_seqno_) + q.front().sequence_length() <= latest_ackno_) {
    q.pop();

    // Reset timer only for successful ACKs
    consecutive_retransmissions_ = 0;
    curr_RTO_ms_ = initial_RTO_ms_;
    retransmission_timer_ms_ = 0;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;
  (void)initial_RTO_ms_;

  retransmission_timer_ms_ += ms_since_last_tick;

  // Timer Expires
  if (curr_RTO_ms_ <= retransmission_timer_ms_ and !q.empty())
  {
    transmit(q.front());

    // Backoff iff window size is non-zero
    if (latest_window_size_ > 0) {
      consecutive_retransmissions_++;
      curr_RTO_ms_ *= 2;
    }

    // Reset timer
    retransmission_timer_ms_ = 0;
  }
}

void TCPSender::send_segment(TCPSenderMessage& msg, const TransmitFunction& transmit) {
  msg.seqno = get_wrapped_next_seqno();
  msg.RST = reader().has_error();

  // Save a copy and transmit to reciever
  q.push(msg);
  transmit(msg);

  next_seqno_ += msg.sequence_length();
}