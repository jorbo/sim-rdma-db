TESTS=$(wildcard */*.csv)
REQ_BINS=$(subst .csv,_req.bin,$(TESTS))
RESP_BINS=$(subst .csv,_resp.bin,$(TESTS))

all: $(REQ_BINS) $(RESP_BINS)

%_req.bin %_resp.bin: %.csv
	./encode.py $^

clean:
	rm -f $(REQ_BINS) $(RESP_BINS)
