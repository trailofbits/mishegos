# This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

from pkg_resources import parse_version
from kaitaistruct import __version__ as ks_version, KaitaiStruct, KaitaiStream, BytesIO
from enum import Enum


if parse_version(ks_version) < parse_version('0.7'):
    raise Exception("Incompatible Kaitai Struct Python API: 0.7 or later is required, but you have %s" % (ks_version))

class Cohort(KaitaiStruct):

    class DecodeStatus(Enum):
        s_none = 0
        s_success = 1
        s_failure = 2
        s_crash = 3
        s_hang = 4
        s_partial = 5
        s_wouldblock = 6
        s_unknown = 7
    def __init__(self, _io, _parent=None, _root=None):
        self._io = _io
        self._parent = _parent
        self._root = _root if _root else self
        self._read()

    def _read(self):
        self.total_cohorts = []
        i = 0
        while not self._io.is_eof():
            self.total_cohorts.append(self._root.Entry(self._io, self, self._root))
            i += 1


    class Entry(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._read()

        def _read(self):
            self.nworkers = self._io.read_u4le()
            self.input_length = self._io.read_u8le()
            self.input = (self._io.read_bytes(self.input_length)).decode(u"ascii")
            self.type = [None] * (self.nworkers)
            for i in range(self.nworkers):
                self.type[i] = self._root.CohortType(self._io, self, self._root)



    class CohortType(KaitaiStruct):
        def __init__(self, _io, _parent=None, _root=None):
            self._io = _io
            self._parent = _parent
            self._root = _root if _root else self
            self._read()

        def _read(self):
            self.status = self._root.DecodeStatus(self._io.read_u4le())
            self.ndecoded = self._io.read_u2le()
            self.workerno = self._io.read_u4le()
            self.worker_so_length = self._io.read_u8le()
            self.worker_so = (self._io.read_bytes(self.worker_so_length)).decode(u"ascii")
            self.len = self._io.read_u2le()
            self.result = (self._io.read_bytes(self.len)).decode(u"ascii")



