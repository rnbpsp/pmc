/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include <tfile.h>
#include <tstring.h>

#include "fileref.h"
#ifdef MPEG_TAG
#include "mpegfile.h"
#endif
#ifdef AAC_TAG
#include "mpegfile.h"
#endif
#ifdef OGG_TAG
#include "vorbisfile.h"
#include "oggflacfile.h"
#include "speexfile.h"
#endif
#ifdef FLAC_TAG
#include "flacfile.h"
#endif
#ifdef MPC_TAG
#include "mpcfile.h"
#endif
#ifdef WAVPACK_TAG
#include "wavpackfile.h"
#endif
#ifdef TTA_TAG
#include "trueaudiofile.h"
#endif

using namespace TagLib;

class FileRef::FileRefPrivate : public RefCounter
{
public:
  FileRefPrivate(File *f) : RefCounter(), file(f) {}
  ~FileRefPrivate() {
    delete file;
  }

  File *file;
  static List<const FileTypeResolver *> fileTypeResolvers;
};

List<const FileRef::FileTypeResolver *> FileRef::FileRefPrivate::fileTypeResolvers;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

FileRef::FileRef()
{
    d = new FileRefPrivate(0);
}

FileRef::FileRef(FileName fileName, bool readAudioProperties,
                 AudioProperties::ReadStyle audioPropertiesStyle)
{
  d = new FileRefPrivate(create(fileName, readAudioProperties, audioPropertiesStyle));
}

FileRef::FileRef(File *file)
{
  d = new FileRefPrivate(file);
}

FileRef::FileRef(const FileRef &ref) : d(ref.d)
{
  d->ref();
}

FileRef::~FileRef()
{
  if(d->deref())
    delete d;
}

Tag *FileRef::tag() const
{
  return d->file->tag();
}

AudioProperties *FileRef::audioProperties() const
{
  return d->file->audioProperties();
}

File *FileRef::file() const
{
  return d->file;
}

bool FileRef::save()
{
  return d->file->save();
}

const FileRef::FileTypeResolver *FileRef::addFileTypeResolver(const FileRef::FileTypeResolver *resolver) // static
{
  FileRefPrivate::fileTypeResolvers.prepend(resolver);
  return resolver;
}

StringList FileRef::defaultFileExtensions()
{
  StringList l;

#ifdef OGG_TAG
  l.append("ogg");
  l.append("spx");
  l.append("oga");
#endif
#ifdef FLAC_TAG
  l.append("flac");
#endif
#ifdef MPEG_TAG
  l.append("mp3");
#endif
#ifdef AAC_TAG
  l.append("aac");
  l.append("mp4");
  l.append("m4a");
#endif
#ifdef MPC_TAG
  l.append("mpc");
#endif
#ifdef WAVPACK_TAG
  l.append("wv");
#endif
#ifdef TTA_TAG
  l.append("tta");
#endif

  return l;
}

bool FileRef::isNull() const
{
  return !d->file || !d->file->isValid();
}

FileRef &FileRef::operator=(const FileRef &ref)
{
  if(&ref == this)
    return *this;

  if(d->deref())
    delete d;

  d = ref.d;
  d->ref();

  return *this;
}

bool FileRef::operator==(const FileRef &ref) const
{
  return ref.d->file == d->file;
}

bool FileRef::operator!=(const FileRef &ref) const
{
  return ref.d->file != d->file;
}

File *FileRef::create(FileName fileName, bool readAudioProperties,
                      AudioProperties::ReadStyle audioPropertiesStyle) // static
{

  List<const FileTypeResolver *>::ConstIterator it = FileRefPrivate::fileTypeResolvers.begin();

  for(; it != FileRefPrivate::fileTypeResolvers.end(); ++it) {
    File *file = (*it)->createFile(fileName, readAudioProperties, audioPropertiesStyle);
    if(file)
      return file;
  }

  // Ok, this is really dumb for now, but it works for testing.

  String s;

#ifdef _WIN32
  s = (wcslen((const wchar_t *) fileName) > 0) ? String((const wchar_t *) fileName) : String((const char *) fileName);
#else
  s = fileName;
#endif

  // If this list is updated, the method defaultFileExtensions() should also be
  // updated.  However at some point that list should be created at the same time
  // that a default file type resolver is created.

  if(s.size() > 4) {
#ifdef OGG_TAG
    if(s.substr(s.size() - 4, 4).upper() == ".OGG")
      return new Ogg::Vorbis::File(fileName, readAudioProperties, audioPropertiesStyle);
	if(s.substr(s.size() - 4, 4).upper() == ".OGA")
      return new Ogg::FLAC::File(fileName, readAudioProperties, audioPropertiesStyle);
	if(s.substr(s.size() - 4, 4).upper() == ".SPX")
      return new Ogg::Speex::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
#ifdef MPEG_TAG
    if(s.substr(s.size() - 4, 4).upper() == ".MP3")
      return new MPEG::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
#ifdef AAC_TAG
	if(s.substr(s.size() - 4, 4).upper() == ".AAC")
      return new MPEG::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
#ifdef FLAC_TAG
    if(s.substr(s.size() - 5, 5).upper() == ".FLAC")
      return new FLAC::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
#ifdef MPC_TAG
    if(s.substr(s.size() - 4, 4).upper() == ".MPC")
      return new MPC::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
#ifdef WAVPACK_TAG
    if(s.substr(s.size() - 3, 3).upper() == ".WV")
      return new WavPack::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
#ifdef TTA_TAG
    if(s.substr(s.size() - 4, 4).upper() == ".TTA")
      return new TrueAudio::File(fileName, readAudioProperties, audioPropertiesStyle);
#endif
  }

  return 0;
}
