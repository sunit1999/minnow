#include "wrapping_integers.hh"

using namespace std;

uint64_t abs_diff(uint64_t a, uint64_t b) {
    return (a > b) ? (a - b) : (b - a);
}

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint32_t seqno = (uint32_t)n + zero_point.raw_value_;
  return Wrap32 { seqno };
}

/*
      seg - 1                seg               seg + 1
----|------------*-  ----|------------*- ----|----------*-
*/
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  (void)zero_point;
  (void)checkpoint;
  uint64_t MOD = 1L << 32; // 2^32
  uint32_t MAX_SEGS = UINT64_MAX/MOD;

  uint32_t offset = raw_value_ - zero_point.raw_value_;

  uint32_t seg = checkpoint/MOD;

  uint64_t res = seg*MOD + offset;

  if (seg > 0) {
    uint64_t res_prev = (seg - 1)*MOD + offset;

    if (abs_diff(res, checkpoint) > abs_diff(res_prev, checkpoint)) {
        res = res_prev;
    }
  }

  if (seg < MAX_SEGS-1) {
    uint64_t res_next = (seg + 1)*MOD + offset;

    if (abs_diff(res, checkpoint) > abs_diff(res_next, checkpoint)) {
      res = res_next;
    }
  }

  return res;
}
