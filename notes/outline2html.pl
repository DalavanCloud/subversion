#!/usr/bin/perl -w

### Convert Emacs outline mode documents to HTML.
### 
### Usage: "htmlize.pl [FILENAME.txt]" to produce FILENAME.html
###     (FILENAME defaults to stdin.)
###
### Note that this is not a general purpose htmlizer, it is tuned for
### inversion.txt. 

use strict;

############ Customizable globals ############

my $list_type = "ul";     # HTML unordered list
my $dest_ext = ".html";

############ End Customizable globals ############


my $source = shift || "-";

my ($base) = split (/\./, $source);
my $dest;
if ((! defined ($base)) or ($base eq "")) {
  $dest = "-"; # default to stdout
}
else {
  $dest = "${base}${dest_ext}";  # otherwise use <FILE>.html
}

my $star_level = 0;     # outline heading level
my $list_level = 0;     
my $inside_pre = 0;     # set to 1 when inside <pre>...</pre> tags

open (SOURCE, "$source") or die ("trouble reading $source ($!)");
open (DEST, ">$dest") or die ("trouble writing $dest ($!)");

# Start off an HTML page with a white background:
print DEST "<html>\n";
print DEST "<body bgcolor=\"#FFFFFF\" fgcolor=\"#000000\">\n";

# Put the outline document into it, htmlifying as we go:
while (<SOURCE>)
{
  my $quoted_mail_line = 0;
  my $num_stars = &count_stars ($_);

  next if (/please be in -\*- outline -\*- mode/);

  if (/^>/) {
    $quoted_mail_line = 1;
  }

  chomp;

  if ($num_stars) {
    # Strip leading stars from heading lines
    $_ =~ s/^\*+ //;
  }

  $_ = &escape_html ($_);

  # Convert "*foo*" to real italics: "<i>foo</i>"
  # todo: make this work for multiple words, not too hard really...
  $_ =~ s/\*([a-zA-Z0-9]+)\*/<i>$1<\/i>/g;

  if ($num_stars == 1)
  {
    $star_level = 1;

    if ($list_level > $num_stars) {
      for (my $l = $list_level; $l > 0; $l--) {
        print DEST "\n</$list_type>\n\n";
        $list_level--;
      }
    }

    print DEST "\n\n<p>\n<hr>\n<p>\n\n";
    print DEST "<center>\n<h1>$_</h1>\n</center>\n";
  }
  elsif ($num_stars == 2)
  {
    $star_level = 2;

    if ($list_level < $num_stars) {
      print DEST "<$list_type>\n\n";
      $list_level = $num_stars;
    }
    elsif ($list_level > $num_stars) {
      print DEST "</$list_type>\n\n";
      $list_level = $num_stars;
    }

    print DEST "<li><strong>$_</strong>\n";
  }
  elsif ($num_stars == 3)
  {
    $star_level = 3;

    if ($list_level < $num_stars) {
      print DEST "<$list_type>\n\n";
      $list_level = $num_stars;
    }
    elsif ($list_level > $num_stars) {
      print DEST "</$list_type>\n\n";
      $list_level = $num_stars;
    }

    print DEST "<li>$_\n";
  }
  else
  {
    if ($star_level)
    {
      if (/^\s+/) {
        print DEST "<pre>\n" if (! $inside_pre);
        $inside_pre = 1;
      }
      elsif ($inside_pre) {
        print DEST "</pre>\n";
        $inside_pre = 0;
      }
    }

    if ($quoted_mail_line) {
      print DEST "<font color=red>$_</font><br>\n";
    }
    elsif ($_ =~ /[a-zA-Z0-9]/) {
      print DEST "$_\n";
    }
    elsif ($_ =~ /^\s*[-_]+\s*$/) {
      print DEST "<p><hr><p>\n";
    }
    else {
      print DEST "\n\n<p>\n\n";
    }
  }
}

close (SOURCE);
close (DEST);


######################## Subroutines ############################

sub escape_html ()
{
  my $str = shift;
  $str =~ s/&/&amp;/g;
  $str =~ s/>/&gt;/g;
  $str =~ s/</&lt;/g;
  return $str;
}


sub count_stars ()
{
  my $str = shift;

  # Handles up to 9 stars.

  return 0 if ($str =~ /^[^*]/); # Common case -- this is not a star line
  return 1 if ($str =~ /^\* /);
  return 2 if ($str =~ /^\*\* /);
  return 3 if ($str =~ /^\*\*\* /);
  return 4 if ($str =~ /^\*\*\*\* /);
  return 5 if ($str =~ /^\*\*\*\*\* /);
  return 6 if ($str =~ /^\*\*\*\*\*\* /);
  return 7 if ($str =~ /^\*\*\*\*\*\*\* /);
  return 8 if ($str =~ /^\*\*\*\*\*\*\*\* /);
  return 9 if ($str =~ /^\*\*\*\*\*\*\*\*\* /);
}
