CSV Format
----------

1. **Opcode:** Name of operation in all caps.
2. **Request Arguments:** Operation specific, struct values are comma separated.
3. **Expected Response:** Operation specific, struct values are comma separated.

The number of columns used will vary depending on the operation. Empty columns
should be omitted (unless I get around to updating the encoder to explicitly
ignore them).
