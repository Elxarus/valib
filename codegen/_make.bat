perl migen_io.pl > mixer_io.cpp
perl migen_ip.pl > mixer_ip.cpp
perl pcm2linear.pl < pcm2linear.template > pcm2linear.cpp
perl linear2pcm.pl < linear2pcm.template > linear2pcm.cpp
perl spk_tblgen.pl > spk_tbl.cpp
perl prime.pl > prime.cpp
