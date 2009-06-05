use strict;

my @chs      = qw(1 2 3 4 5 6);
my @formats  = qw(FORMAT_PCM16 FORMAT_PCM24 FORMAT_PCM32 FORMAT_PCM16_BE FORMAT_PCM24_BE FORMAT_PCM32_BE FORMAT_PCMFLOAT FORMAT_PCMDOUBLE);
my @names    = qw(pcm16    pcm24    pcm32    pcm16_be pcm24_be pcm32_be pcmfloat pcmdouble );
my @types    = qw(int16_t  int24_t  int32_t  int16_t  int24_t  int32_t  float    double    );
my @funcs    = qw(le2int16 le2int24 le2int32 be2int16 be2int24 be2int32 sample_t sample_t  );

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
    print "void ${name}_linear_${ch}ch(uint8_t *, samples_t, size_t);\n";
  }
  print "\n";
}

###############################################################################
# array of functions

print "typedef void (Converter::*convert_t)(uint8_t *rawdata, samples_t samples, size_t size);\n\n";
print "static const int formats_tbl[] = { ".join(", ", @formats)." };\n\n";
print "static const int formats = ".join(" | ", @formats).";\n\n";

print "static const convert_t pcm2linear_tbl[NCHANNELS][".($#formats+1)."] = {\n";
foreach $ch (@chs)
{
  print " { ";
  print join ", ", map { "${_}_linear_${ch}ch" } @names;
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
    $convert = $convert."    dst[$_][0] = $func(src[$_]); dst[$_]++;\n" foreach (0..$ch-1);
    $text = join('', @template);
    $text =~ s/(\$\w+)/$1/gee;

    print "void\n";
    print "${name}_linear_${ch}ch(uint8_t *rawdata, samples_t samples, size_t size)\n";
    print $text;
  }
  print "\n";
}
