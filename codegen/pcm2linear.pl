use strict;

my @chs      = qw(1 2 3 4 5 6);
my @formats  = qw(FORMAT_PCM16 FORMAT_PCM24 FORMAT_PCM32 FORMAT_PCMFLOAT FORMAT_PCM16_LE FORMAT_PCM24_LE FORMAT_PCM32_LE FORMAT_PCMFLOAT_LE );
my @names    = qw(pcm16    pcm24    pcm32    pcmfloat pcm16_le pcm24_le pcm32_le pcmfloat_le );
my @types    = qw(int16_t  int24_t  int32_t  float    int16_t  int24_t  int32_t  float       );
my @funcs    = qw(sample_t sample_t sample_t sample_t swab_s16 swab_s24 swab_s32 swab_float  );

my $ch;
my $i;
my $format;
my $name;
my $type;
my $func;

my @template = <>;
my $convert;
my $text;


###############################################################################
# class members

foreach $ch (@chs)
{
  foreach $name (@names)
  {
    print "  bool ".$name."_linear_".$ch."ch(Chunk *);\n";
  }
  print "\n";
}

###############################################################################
# array of functions

print "typedef bool (Converter::*convert_t)(Chunk *);\n\n";
print "static const int formats_tbl[] = {".join(", ", @formats)."}\n\n";
print "const int formats = ".join(" | ", @formats).";\n\n";

print "static const convert_t pcm2linear_tbl[NCHANNELS][".($#formats+1)."] = {\n";
foreach $ch (@chs)
{
  print " { ";
  print join ", ", map { "Converter::".$_."_linear_".$ch."ch" } @names;
  print " },\n";
}
print "};\n\n";

###############################################################################
# function implementation

foreach $ch (@chs)
{
  for ($i = 0; $i <= $#formats; $i++)
  {
    $format = $formats[$i];
    $name = $names[$i];
    $type = $types[$i];
    $func = $funcs[$i];
    $convert = "";
    $convert = $convert."    dst[nsamples * $_] = $func(src[$_]);\n" foreach (0..$ch-1);
    $text = join('', @template);
    $text =~ s/(\$\w+)/$1/gee;

    print "bool\n";
    print "Converter::".$name."_linear_".$ch."ch(Chunk *_chunk)\n";
    print $text;
  }
  print "\n";
}
