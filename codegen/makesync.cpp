#include <iostream>
#include <string>
#include "syncscan.h"

using namespace std;

int main()
{
  uint32_t i;

  // AC3 trie
  SyncTrie AC3Trie = SyncTrie(0x0b77, 16) | SyncTrie(0x770b, 16);
  AC3Trie.optimize();
  cout << "AC3: " << AC3Trie.serialize() << endl;

  // DTS trie
  SyncTrie DTSTrie =
    SyncTrie(0x7ffe8001, 32) | SyncTrie(0xfe7f0180, 32) |
    SyncTrie(0x1fffe800, 32) | SyncTrie(0xff1f00e8, 32);
  DTSTrie.optimize();
  cout << "DTS: " << DTSTrie.serialize() << endl;

  // MPATrie
  SyncTrie MPAVersion = SyncTrie(0x0, 2) | SyncTrie(0x2, 2) | SyncTrie(0x3, 2);

  SyncTrie MPALayer = SyncTrie(0x1, 2) | SyncTrie(0x2, 2) | SyncTrie(0x3, 2);

  SyncTrie MPABitrate;
  for (i = 0; i < 16; i++)
    MPABitrate |= SyncTrie(i, 4);

  SyncTrie MPARate = SyncTrie(0x0, 2) | SyncTrie(0x1, 2) | SyncTrie(0x2, 2);

  // MPEG2.5 support only for big endian
  SyncTrie MPATrieBigEndian =
    SyncTrie(0x7ff, 11) + // Sync
    MPAVersion +    // version with MPEG2.5 support
    MPALayer +      // layer != 0
    SyncTrie::any + // protection
    MPABitrate +    // bitrate != 0xf
    MPARate;        // rate != 0x3

  SyncTrie MPATrieLittleEndian =
    SyncTrie(0xf, 4) + // Sync
    SyncTrie::any + // version w/o MPEG2.5 support
    MPALayer +      // layer != 0
    SyncTrie::any + // protection
    SyncTrie(0xff, 8) + // Sync
    SyncTrie("xxxxxxxx") +
    MPABitrate +    // bitrate != 0xf
    MPARate;        // rate != 0x3

  SyncTrie MPATrie = MPATrieBigEndian | MPATrieLittleEndian;
  MPATrie.optimize();
  cout << "MPA: " << MPATrie.serialize() << endl;

  ///////////////////////////////////////////////////////////////////////////////
  // AAC ADTS header definition
  //
  // A: 12 syncword = 0xfff
  // B: 1  id
  // C: 2  layer = 0
  // D: 1  protection_absent
  // E: 2  object_type
  // F: 4  sampling_frequency_index < 0xc
  // G: 1  private_bit
  // H: 3  channel_configuration != 0
  //
  //                 A            B C  D E
  //                 v            v v  v v
  SyncTrie ADTSTrie("iiiiiiiiiiii x oo x xA");

  SyncTrie ADTSRate;
  for (i = 0; i < 0xc; i++)
    ADTSRate |= SyncTrie(i, 4);

  SyncTrie ADTSChannels;
  for (i = 1; i < 8; i++)
    ADTSChannels |= SyncTrie(i, 3);

  ADTSTrie += ADTSRate + SyncTrie::any + ADTSChannels;
  ADTSTrie.optimize();
  cout << "AAC ADTS: " << ADTSTrie.serialize() << endl;

  /////////////////////////////////////////////////////////////////////////////
  // MPEG PES
  // A: 24 - syncword = 0x00 0x00 0x01
  // B: 8  - stream number >= 0xB9

  SyncTrie PESTrie("oooo oooo oooo oooo oooo oooI");

  SyncTrie PESStream;
  for (i = 0xb9; i <= 0xff; i++)
    PESStream |= SyncTrie(i, 8);

  PESTrie += PESStream;
  PESTrie.optimize();

  cout << "MPEG PES: " << PESTrie.serialize() << endl;

  return 0;
}
