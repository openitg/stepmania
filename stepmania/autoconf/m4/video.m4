AC_DEFUN([SM_VIDEO], [
AC_ARG_WITH(ffmpeg, AS_HELP_STRING([--without-ffmpeg],[Disable ffmpeg support]), with_ffmpeg=$withval, with_ffmpeg=yes)

old_LIBS="$LIBS"
old_CFLAGS="$CFLAGS"
old_CXXFLAGS="$CXXFLAGS"

if test "$with_ffmpeg" != "no"; then
	if test "$with_ffmpeg" != "yes"; then
		if test $(echo $with_ffmpeg|cut -c 1) != /; then
			with_ffmpeg=$(/bin/pwd)/$with_ffmpeg
		fi
		CFLAGS="-I$with_ffmpeg/include $CFLAGS"
		CXXFLAGS="-I$with_ffmpeg/include $CXXFLAGS"
		LIBS="$with_ffmpeg/lib/libavutil.a $LIBS"
		AC_CHECK_FUNC([av_free], have_libavutil=yes, have_libavutil=no)
		LIBS="$with_ffmpeg/lib/libavcodec.a $LIBS"
		AC_CHECK_FUNC([avcodec_init], have_libavcodec=yes, have_libavcodec=no)
		LIBS="$with_ffmpeg/lib/libavformat.a $LIBS"
		AC_CHECK_FUNC([guess_format], have_libavformat=yes, have_libavformat=no)
	else
		AC_SEARCH_LIBS(av_free, [avutil], have_libavutil=yes,  have_libavutil=no)
		AC_SEARCH_LIBS(avcodec_init, [avcodec], have_libavcodec=yes,  have_libavcodec=no)
		AC_SEARCH_LIBS(guess_format, [avformat], have_libavformat=yes,  have_libavformat=no)
	fi

if test "$have_libavcodec" = "yes"; then
  AC_MSG_CHECKING([for matching libavcodec headers and libs])
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
	#include <ffmpeg/avcodec.h>
	int main()
	{
		return ( LIBAVCODEC_VERSION_INT == avcodec_version() &&
			 LIBAVCODEC_BUILD == avcodec_build() ) ? 0:1;
	}
	]])],[],[have_libavcodec=no],[])
  AC_MSG_RESULT($have_libavcodec)
  if test "$have_libavcodec" = "yes"; then
    AC_MSG_CHECKING([libavcodec revision])
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
	#include <ffmpeg/avcodec.h>
	int main()
	{
		return ( LIBAVCODEC_VERSION_INT == 0x332800 ) ? 0:1;
	}
	]])],[],[have_libavcodec=no],[])
    AC_MSG_RESULT($have_libavcodec)
  fi
fi

if test "$have_libavformat" = "yes"; then
  AC_MSG_CHECKING([libavformat revision])
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
	#include <ffmpeg/avformat.h>
	int main()
	{
		return ( LIBAVFORMAT_VERSION_INT == 0x330B00 )? 0:1;
	}
	]])],[],[have_libavformat=no],[])
  AC_MSG_RESULT($have_libavformat)
fi
fi

have_ffmpeg=no
if test "$have_libavutil" = "yes" -a "$have_libavformat" = "yes" -a "$have_libavcodec" = "yes"; then
	have_ffmpeg=yes
	AC_DEFINE(HAVE_FFMPEG, 1, [FFMPEG support available])
else
	LIBS="$old_LIBS"
	CFLAGS="$old_CFLAGS"
	CXXFLAGS="$old_CXXFLAGS"
fi
AM_CONDITIONAL(HAVE_FFMPEG, test "$have_ffmpeg" = "yes")



AC_ARG_WITH(theora, AS_HELP_STRING([--without-theora],[Disable Theora support]), with_theora=$withval, with_theora=yes)

have_vorbis=no
if test "$with_theora" = "yes"; then
	AC_CHECK_LIB(ogg, ogg_stream_init, have_libogg=yes, have_libogg=no)
	AC_CHECK_LIB(theora, theora_decode_init, have_libtheora=yes, have_libtheora=no, [-logg])
	if test "$have_libtheora" = "yes" -a "$have_libogg" = "yes" -a "$have_ffmpeg" = "yes"; then
		have_theora=yes
		LIBS="$LIBS -ltheora -logg"
		AC_DEFINE(HAVE_THEORA, 1, [Theora support available])
	else
		echo Not all theora libraries found.

		# Special note for unintuitive requirement (for color space conversion).
		if test "$have_libtheora" = "yes" -a "$have_libogg" = "yes"; then
			echo Ogg Theora support requires ffmpeg.
		fi
	fi

fi

AM_CONDITIONAL(HAVE_THEORA, test "$have_theora" = "yes" )

])
