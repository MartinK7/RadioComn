#define bitRead(_register_, _bit_)				(_register_ & (1<<_bit_))
#define bitSet(_register_, _bit_)				(_register_ |= (1<<_bit_))
#define bitClear(_register_, _bit_)				(_register_ &= ~(1<<_bit_))
#define bitToggle(_register_, _bit_)			(_register_ ^= (1<<_bit_))
#define bitWrite(_register_, _bit_, _state_)	if(_state_){_register_|=(1<<_bit_);}else{_register_&=~(1<<_bit_);}