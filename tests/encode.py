#!/usr/bin/env python3
from sys import argv, exit, stderr
from treetypes import *


def parse_req_resp(req_resp_str):
	split = req_resp_str[:-1].split(",")
	opcode = split[0]
	match opcode:
		case Opcode.NOP.name:
			return (nop_req(), nop_resp())
		case Opcode.SEARCH.name:
			return (
				search_req(int(split[1])),
				search_resp(ErrorCode[split[2]].value, ErrorCode[split[2]].value)
			)
		case Opcode.INSERT.name:
			return (
				insert_req(int(split[1]), int(split[2])),
				insert_resp(ErrorCode[split[3]].value)
			)


if __name__ == '__main__':
	if len(argv) < 2:
		print("Need to give a filename", file=stderr)
		exit(1)
	else:
		fname_in = argv[1]
		fname_req = f"{argv[1][:-4]}_req.bin"
		fname_resp = f"{argv[1][:-4]}_resp.bin"
		with open(fname_in) as fin:
			lines = fin.readlines()
		with open(fname_req, "wb") as fout_req:
			with open(fname_resp, "wb") as fout_resp:
				for line in lines:
					req, resp = parse_req_resp(line)
					fout_req.write(req)
					fout_resp.write(resp)
