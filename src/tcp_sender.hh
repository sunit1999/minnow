#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ),
      isn_( isn ), 
      initial_RTO_ms_( initial_RTO_ms ), 
      curr_RTO_ms_( initial_RTO_ms ),
      q()
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }
  Reader& reader() { return input_.reader(); }

  Wrap32 get_wrapped_next_seqno() const {
    return Wrap32::wrap(next_seqno_, isn_);
  }

  // Utility method to save and transmit a segment
  void send_segment(TCPSenderMessage& msg, const TransmitFunction& transmit);

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t next_seqno_ = 0;    // next absolute seqno to be sent
  uint64_t latest_ackno_ = 0;  // max ackno recieved
  uint16_t latest_window_size_ = 1;  // window size advertised by reciever
  uint64_t consecutive_retransmissions_ = 0;
  uint64_t retransmission_timer_ms_ = 0;  // Time in ms since any segment was sent
  uint64_t curr_RTO_ms_;  // Threshold for retransmission timer, can backoff

  bool syn_sent = false;
  bool fin_sent = false;

  std::queue<TCPSenderMessage> q; // Save segments locally to track unacknowledged segments
};
