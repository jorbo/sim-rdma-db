from enum import Enum
from struct import pack


_uint8_t = "B"
_int32_t = "i"
_uint32_t = "I"

_bkey_t = _uint32_t
_bval_t = _int32_t
_ErrorCode = _uint8_t
_Opcode = _uint8_t

_bstatusval_t = _ErrorCode + _bval_t
_KvPair = _bkey_t + _bval_t

insert_in_t = _KvPair
insert_out_t = _ErrorCode
search_in_t = _bkey_t
search_out_t = _bstatusval_t


class ErrorCode(Enum):
	SUCCESS = 0
	KEY_EXISTS = 1
	NOT_IMPLEMENTED = 2
	NOT_FOUND = 3
	INVALID_ARGUMENT = 4
	OUT_OF_MEMORY = 5
	PARENT_FULL = 6

class Opcode(Enum):
	NOP = 0
	SEARCH = 1
	INSERT = 2


def nop_req():
	return pack(f"={_Opcode}8x", Opcode.NOP.value)
def search_req(key):
	return pack(f"={_Opcode}{search_in_t}4x", Opcode.SEARCH.value, key)
def insert_req(key, value):
	return pack(f"={_Opcode}{insert_in_t}", Opcode.INSERT.value, key, value)

def nop_resp():
	return pack(f"={_Opcode}5x", Opcode.NOP.value)
def search_resp(status, value):
	return pack(f"={_Opcode}{search_out_t}", Opcode.SEARCH.value, status, value)
def insert_resp(status):
	return pack(f"={_Opcode}{insert_out_t}4x", Opcode.INSERT.value, status)
