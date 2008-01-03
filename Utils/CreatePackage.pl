#!/usr/bin/perl
use strict;
use warnings;

my $CWD = `pwd`;
chomp $CWD;
my $ZIP = "zip";
if ( -x "$CWD/zip.exe" ) { $ZIP = "$CWD/zip.exe"; }
	
if( $#ARGV < 1 )
{
	print "Usage: create-zips.pl <game directory> <zip directory>\n";
	exit 1;
}

my $in = $ARGV[0];
my $out = $ARGV[1];

if( ! -d $in )
{
	print "$in isn't a directory\n";
	exit 1;
}
if( ! -d $out )
{
	print "$out isn't a directory\n";
	exit 1;
}

# Resolve $out to an absolute path.
my $pwd = `pwd`;
chomp $pwd;
chdir $out || die "chdir($out): $!";
$out = `pwd`;
chomp $out;
chdir $pwd || die "chdir($pwd): $!";

print "$in -> $out\n";

sub ZipFiles($$@);
sub ZipFiles($$@)
{
	my $dir = shift;
	my $file = shift;
	my @files = @{(shift)};

	$dir = $in . "/" . $dir;
	chdir $dir || die "chdir($dir): $!";

	my @list;

	foreach my $x ( @files )
	{
		print "$x\n";
		# No CVS directories, no dotfiles, files only.
		my $output = `find "$x" -path '*/CVS' -prune -o -path '*/.*' -prune -o '(' -type f -a -name '*' ')' -print`;
		if( $? == -1 ) { die "Find failed: $!\n"; }
		if( $? & 127 ) { die "Find failed with signal %d\n", ($? & 127); }
		if( $? >> 8 != 0 ) { exit 1; }

		push @list, split(/\n/, $output);
	}

	# ZIP the files.
	{
		my @lst;
		my %lst_hash;
		for( my $f = 0; $f <= $#list; ++$f )
		{
			push @lst, $list[$f];
			$lst_hash{$list[$f]} = 1;
		}
		
		# @lst contains the files for zip number $zipno.

		my $filename = $out . "/$file.smzip";
		# If the ZIP already exists, get a list of files in it.  Delete any files
		# that shouldn't be there.
		if( -e $filename )
		{
			my @files = split(/\n/, `unzip -l "$filename"`);
			shift @files; # "Archive: ..."
			shift @files; # " Length     Date   Time    Name"
			shift @files; # "--------    ----   ----    ----"
			pop @files;   # "--------    ----   ----    -------"
			pop @files;   # "    2501                   2 files"

			foreach my $fn ( @files )
			{
				# "     1081  02-11-04 19:27   foo" -> "foo"
				$fn = substr( $fn, 28 );
			}

			# Loop over files in the ZIP, and find any files not in %lst_hash.
			my @unneeded;
			for( my $n = 0; $n <= $#files; ++$n )
			{
				my $file = $files[$n];
				# Skip directories.
				if( $file =~ /.*\/$/ ) { next; }
				if( defined( $lst_hash{$file} ) ) { next; }
				push @unneeded, $files[$n];
			}

			if( $#unneeded != -1 )
			{
				print "$filename has " . ($#unneeded+1) . " extra files; removing ...\n";

				open TMP, ">/tmp/zip.lst" || die "$!";
				foreach my $fn ( @unneeded ) { print TMP "$fn\n"; }
				close TMP;

				system "$ZIP -b $out $filename -d -@ < /tmp/zip.lst";
				if ($? == -1) { die "Zip failed: $!\n"; }
				if ($? & 127) { die "Zip failed with signal %d\n", ($? & 127); }
				if( $? >> 8 != 0 ) { die "Zip failed\n"; }
			}
		}

		if( -e $filename ) { print "Updating $filename ...\n"; }
		else { print "Creating $filename ...\n"; }

		# -q: quiet
		# -u: update only (only if it already exists)
		# -y: store links
		# -n: don't compress

		my $options = "-q -y -n .ogg:.mp3:.png:.jpg:.mpg:.avi";
		if( -e $filename ) { $options .= " -u"; }
		open TMP, ">/tmp/zip.lst" || die "$!";
		foreach my $fn ( @lst ) { print TMP "$fn\n"; }
		close TMP;

		system "$ZIP $options -b $out $filename -@ < /tmp/zip.lst" || die "Zip failed";

		if ($? == -1) { die "Zip failed: $!\n"; }
		if ($? & 127) { die "Zip failed with signal %d\n", ($? & 127); }
		my $ret = ($? >> 8);
		if( $ret != 0 && $ret != 12 ) { die "Zip failed\n"; }
	}

	chdir $CWD || die "chdir($CWD): $!";
}


my @files = (
	"BackgroundEffects",
	"BackgroundTransitions",
	"BGAnimations",
	"Characters",
	"Courses/Samples",
	"Data",
	"NoteSkins/common/common",
	"NoteSkins/dance/default",
	"NoteSkins/dance/bold",
	"NoteSkins/dance/flat",
	"NoteSkins/pump/Classic",
	"NoteSkins/pump/default",
	"Themes/default",
	"Themes/pump",
	"Themes/beat"
);

ZipFiles( ".", "StepMania", \@files );

