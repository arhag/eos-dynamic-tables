# eos-dynamic-tables
The purpose of this repository is to experiment with and develop a proof-of-concept of the dynamic tables feature for EOS.IO
(as well as the new type system and serialization tools on which it depends) before eventually beginning the process of 
merging the developed code into [EOS.IO](https://github.com/EOSIO/eos) code base.

As of now the only documentation on the current design is available in the comments in [this GitHub issue](https://github.com/EOSIO/eos/issues/354).
The comments in the current code are very limited but will be added over time (using Doxygen) and formal documentation on the type system and ABI,
as well as tutorials meant for contract developers on how to use the serialization/deserialization tools and interact with the dynamic table system will eventually be written.

Currently, there are two example programs, `reflection_test1` and `reflection_test2`, which demonstrate some of the existing features of the new type system, 
the C++ serialization/deserialization tools, and the ability to compare two instances of comparable objects directly using their raw data in serialized form.

To compile simply use `cmake` and then `make`. The external dependencies (other than the C++ STL) include a few Boost libraries.

