local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")
local O = require("subsonic_api.operations")

---@class subsonic_api.Models
---@field Agent fun(value: subsonic_api.models.Agent): subsonic_api.models.Agent
---@field AlbumID3 fun(value: subsonic_api.models.AlbumID3): subsonic_api.models.AlbumID3
---@field AlbumID3WithSongs fun(value: subsonic_api.models.AlbumID3WithSongs): subsonic_api.models.AlbumID3WithSongs
---@field AlbumList2 fun(value: subsonic_api.models.AlbumList2): subsonic_api.models.AlbumList2
---@field AlbumListType fun(value: subsonic_api.models.AlbumListType): subsonic_api.models.AlbumListType
---@field ArtistID3 fun(value: subsonic_api.models.ArtistID3): subsonic_api.models.ArtistID3
---@field ArtistWithAlbumsID3 fun(value: subsonic_api.models.ArtistWithAlbumsID3): subsonic_api.models.ArtistWithAlbumsID3
---@field ArtistsID3 fun(value: subsonic_api.models.ArtistsID3): subsonic_api.models.ArtistsID3
---@field Child fun(value: subsonic_api.models.Child): subsonic_api.models.Child
---@field Contributor fun(value: subsonic_api.models.Contributor): subsonic_api.models.Contributor
---@field Cue fun(value: subsonic_api.models.Cue): subsonic_api.models.Cue
---@field CueLine fun(value: subsonic_api.models.CueLine): subsonic_api.models.CueLine
---@field DiscTitle fun(value: subsonic_api.models.DiscTitle): subsonic_api.models.DiscTitle
---@field ExplicitStatus fun(value: subsonic_api.models.ExplicitStatus): subsonic_api.models.ExplicitStatus
---@field GenericMediaType fun(value: subsonic_api.models.GenericMediaType): subsonic_api.models.GenericMediaType
---@field GetAlbumList2Response fun(value: subsonic_api.models.GetAlbumList2Response): subsonic_api.models.GetAlbumList2Response
---@field GetAlbumResponse fun(value: subsonic_api.models.GetAlbumResponse): subsonic_api.models.GetAlbumResponse
---@field GetArtistResponse fun(value: subsonic_api.models.GetArtistResponse): subsonic_api.models.GetArtistResponse
---@field GetArtistsResponse fun(value: subsonic_api.models.GetArtistsResponse): subsonic_api.models.GetArtistsResponse
---@field GetLyricsBySongIdResponse fun(value: subsonic_api.models.GetLyricsBySongIdResponse): subsonic_api.models.GetLyricsBySongIdResponse
---@field GetLyricsResponse fun(value: subsonic_api.models.GetLyricsResponse): subsonic_api.models.GetLyricsResponse
---@field GetMusicFoldersResponse fun(value: subsonic_api.models.GetMusicFoldersResponse): subsonic_api.models.GetMusicFoldersResponse
---@field GetOpenSubsonicExtensionsResponse fun(value: subsonic_api.models.GetOpenSubsonicExtensionsResponse): subsonic_api.models.GetOpenSubsonicExtensionsResponse
---@field GetPlaylistResponse fun(value: subsonic_api.models.GetPlaylistResponse): subsonic_api.models.GetPlaylistResponse
---@field GetPlaylistsResponse fun(value: subsonic_api.models.GetPlaylistsResponse): subsonic_api.models.GetPlaylistsResponse
---@field GetRandomSongsResponse fun(value: subsonic_api.models.GetRandomSongsResponse): subsonic_api.models.GetRandomSongsResponse
---@field GetScanStatusResponse fun(value: subsonic_api.models.GetScanStatusResponse): subsonic_api.models.GetScanStatusResponse
---@field GetSongResponse fun(value: subsonic_api.models.GetSongResponse): subsonic_api.models.GetSongResponse
---@field GetStarred2Response fun(value: subsonic_api.models.GetStarred2Response): subsonic_api.models.GetStarred2Response
---@field IndexID3 fun(value: subsonic_api.models.IndexID3): subsonic_api.models.IndexID3
---@field ItemDate fun(value: subsonic_api.models.ItemDate): subsonic_api.models.ItemDate
---@field ItemGenre fun(value: subsonic_api.models.ItemGenre): subsonic_api.models.ItemGenre
---@field Line fun(value: subsonic_api.models.Line): subsonic_api.models.Line
---@field Lyrics fun(value: subsonic_api.models.Lyrics): subsonic_api.models.Lyrics
---@field LyricsList fun(value: subsonic_api.models.LyricsList): subsonic_api.models.LyricsList
---@field MediaType fun(value: subsonic_api.models.MediaType): subsonic_api.models.MediaType
---@field MusicFolder fun(value: subsonic_api.models.MusicFolder): subsonic_api.models.MusicFolder
---@field MusicFolders fun(value: subsonic_api.models.MusicFolders): subsonic_api.models.MusicFolders
---@field OpenSubsonicExtension fun(value: subsonic_api.models.OpenSubsonicExtension): subsonic_api.models.OpenSubsonicExtension
---@field Playlist fun(value: subsonic_api.models.Playlist): subsonic_api.models.Playlist
---@field PlaylistWithSongs fun(value: subsonic_api.models.PlaylistWithSongs): subsonic_api.models.PlaylistWithSongs
---@field Playlists fun(value: subsonic_api.models.Playlists): subsonic_api.models.Playlists
---@field RecordLabel fun(value: subsonic_api.models.RecordLabel): subsonic_api.models.RecordLabel
---@field ReplayGain fun(value: subsonic_api.models.ReplayGain): subsonic_api.models.ReplayGain
---@field ScanStatus fun(value: subsonic_api.models.ScanStatus): subsonic_api.models.ScanStatus
---@field Search3Response fun(value: subsonic_api.models.Search3Response): subsonic_api.models.Search3Response
---@field SearchResult3 fun(value: subsonic_api.models.SearchResult3): subsonic_api.models.SearchResult3
---@field Songs fun(value: subsonic_api.models.Songs): subsonic_api.models.Songs
---@field Starred2 fun(value: subsonic_api.models.Starred2): subsonic_api.models.Starred2
---@field StructuredLyrics fun(value: subsonic_api.models.StructuredLyrics): subsonic_api.models.StructuredLyrics
---@field SubsonicError fun(value: subsonic_api.models.SubsonicError): subsonic_api.models.SubsonicError
---@field SubsonicResponse fun(value: subsonic_api.models.SubsonicResponse): subsonic_api.models.SubsonicResponse

---@class subsonic_api.Operations
---@field get_album fun(params: subsonic_api.get_albumParams): subsonic_api.get_albumOperation
---@field get_album_list2 fun(params: subsonic_api.get_album_list2Params): subsonic_api.get_album_list2Operation
---@field get_artist fun(params: subsonic_api.get_artistParams): subsonic_api.get_artistOperation
---@field get_artists fun(params: subsonic_api.get_artistsParams): subsonic_api.get_artistsOperation
---@field get_cover_art fun(params: subsonic_api.get_cover_artParams): subsonic_api.get_cover_artOperation
---@field get_lyrics fun(params: subsonic_api.get_lyricsParams): subsonic_api.get_lyricsOperation
---@field get_lyrics_by_song_id fun(params: subsonic_api.get_lyrics_by_song_idParams): subsonic_api.get_lyrics_by_song_idOperation
---@field get_music_folders fun(params: subsonic_api.get_music_foldersParams): subsonic_api.get_music_foldersOperation
---@field get_open_subsonic_extensions fun(params: subsonic_api.get_open_subsonic_extensionsParams): subsonic_api.get_open_subsonic_extensionsOperation
---@field get_playlist fun(params: subsonic_api.get_playlistParams): subsonic_api.get_playlistOperation
---@field get_playlists fun(params: subsonic_api.get_playlistsParams): subsonic_api.get_playlistsOperation
---@field get_random_songs fun(params: subsonic_api.get_random_songsParams): subsonic_api.get_random_songsOperation
---@field get_scan_status fun(params: subsonic_api.get_scan_statusParams): subsonic_api.get_scan_statusOperation
---@field get_song fun(params: subsonic_api.get_songParams): subsonic_api.get_songOperation
---@field get_starred2 fun(params: subsonic_api.get_starred2Params): subsonic_api.get_starred2Operation
---@field ping fun(params: subsonic_api.pingParams): subsonic_api.pingOperation
---@field post_get_album fun(params: subsonic_api.post_get_albumParams): subsonic_api.post_get_albumOperation
---@field post_get_album_list2 fun(params: subsonic_api.post_get_album_list2Params): subsonic_api.post_get_album_list2Operation
---@field post_get_artist fun(params: subsonic_api.post_get_artistParams): subsonic_api.post_get_artistOperation
---@field post_get_artists fun(params: subsonic_api.post_get_artistsParams): subsonic_api.post_get_artistsOperation
---@field post_get_lyrics fun(params: subsonic_api.post_get_lyricsParams): subsonic_api.post_get_lyricsOperation
---@field post_get_lyrics_by_song_id fun(params: subsonic_api.post_get_lyrics_by_song_idParams): subsonic_api.post_get_lyrics_by_song_idOperation
---@field post_get_music_folders fun(params: subsonic_api.post_get_music_foldersParams): subsonic_api.post_get_music_foldersOperation
---@field post_get_open_subsonic_extensions fun(params: subsonic_api.post_get_open_subsonic_extensionsParams): subsonic_api.post_get_open_subsonic_extensionsOperation
---@field post_get_playlist fun(params: subsonic_api.post_get_playlistParams): subsonic_api.post_get_playlistOperation
---@field post_get_playlists fun(params: subsonic_api.post_get_playlistsParams): subsonic_api.post_get_playlistsOperation
---@field post_get_random_songs fun(params: subsonic_api.post_get_random_songsParams): subsonic_api.post_get_random_songsOperation
---@field post_get_scan_status fun(params: subsonic_api.post_get_scan_statusParams): subsonic_api.post_get_scan_statusOperation
---@field post_get_song fun(params: subsonic_api.post_get_songParams): subsonic_api.post_get_songOperation
---@field post_get_starred2 fun(params: subsonic_api.post_get_starred2Params): subsonic_api.post_get_starred2Operation
---@field post_ping fun(params: subsonic_api.post_pingParams): subsonic_api.post_pingOperation
---@field post_search3 fun(params: subsonic_api.post_search3Params): subsonic_api.post_search3Operation
---@field post_star fun(params: subsonic_api.post_starParams): subsonic_api.post_starOperation
---@field post_unstar fun(params: subsonic_api.post_unstarParams): subsonic_api.post_unstarOperation
---@field search3 fun(params: subsonic_api.search3Params): subsonic_api.search3Operation
---@field star fun(params: subsonic_api.starParams): subsonic_api.starOperation
---@field stream fun(params: subsonic_api.streamParams): subsonic_api.streamOperation
---@field unstar fun(params: subsonic_api.unstarParams): subsonic_api.unstarOperation

---@class subsonic_api.Client
---@field operations subsonic_api.Operations
---@field models subsonic_api.Models
---@field private options subsonic_api.Options
local Client = {}
Client.__index = Client

---@param options subsonic_api.Options
---@return subsonic_api.Client
function Client.new(options)
    R.options(options)
    local self = setmetatable({ options = options, models = {}, operations = {} }, Client)
    self.models.Agent = function(value) return R.validate(V.check_Agent, options.json, value) end
    self.models.AlbumID3 = function(value) return R.validate(V.check_AlbumID3, options.json, value) end
    self.models.AlbumID3WithSongs = function(value) return R.validate(V.check_AlbumID3WithSongs, options.json, value) end
    self.models.AlbumList2 = function(value) return R.validate(V.check_AlbumList2, options.json, value) end
    self.models.AlbumListType = function(value) return R.validate(V.check_AlbumListType, options.json, value) end
    self.models.ArtistID3 = function(value) return R.validate(V.check_ArtistID3, options.json, value) end
    self.models.ArtistWithAlbumsID3 = function(value) return R.validate(V.check_ArtistWithAlbumsID3, options.json, value) end
    self.models.ArtistsID3 = function(value) return R.validate(V.check_ArtistsID3, options.json, value) end
    self.models.Child = function(value) return R.validate(V.check_Child, options.json, value) end
    self.models.Contributor = function(value) return R.validate(V.check_Contributor, options.json, value) end
    self.models.Cue = function(value) return R.validate(V.check_Cue, options.json, value) end
    self.models.CueLine = function(value) return R.validate(V.check_CueLine, options.json, value) end
    self.models.DiscTitle = function(value) return R.validate(V.check_DiscTitle, options.json, value) end
    self.models.ExplicitStatus = function(value) return R.validate(V.check_ExplicitStatus, options.json, value) end
    self.models.GenericMediaType = function(value) return R.validate(V.check_GenericMediaType, options.json, value) end
    self.models.GetAlbumList2Response = function(value) return R.validate(V.check_GetAlbumList2Response, options.json, value) end
    self.models.GetAlbumResponse = function(value) return R.validate(V.check_GetAlbumResponse, options.json, value) end
    self.models.GetArtistResponse = function(value) return R.validate(V.check_GetArtistResponse, options.json, value) end
    self.models.GetArtistsResponse = function(value) return R.validate(V.check_GetArtistsResponse, options.json, value) end
    self.models.GetLyricsBySongIdResponse = function(value) return R.validate(V.check_GetLyricsBySongIdResponse, options.json, value) end
    self.models.GetLyricsResponse = function(value) return R.validate(V.check_GetLyricsResponse, options.json, value) end
    self.models.GetMusicFoldersResponse = function(value) return R.validate(V.check_GetMusicFoldersResponse, options.json, value) end
    self.models.GetOpenSubsonicExtensionsResponse = function(value) return R.validate(V.check_GetOpenSubsonicExtensionsResponse, options.json, value) end
    self.models.GetPlaylistResponse = function(value) return R.validate(V.check_GetPlaylistResponse, options.json, value) end
    self.models.GetPlaylistsResponse = function(value) return R.validate(V.check_GetPlaylistsResponse, options.json, value) end
    self.models.GetRandomSongsResponse = function(value) return R.validate(V.check_GetRandomSongsResponse, options.json, value) end
    self.models.GetScanStatusResponse = function(value) return R.validate(V.check_GetScanStatusResponse, options.json, value) end
    self.models.GetSongResponse = function(value) return R.validate(V.check_GetSongResponse, options.json, value) end
    self.models.GetStarred2Response = function(value) return R.validate(V.check_GetStarred2Response, options.json, value) end
    self.models.IndexID3 = function(value) return R.validate(V.check_IndexID3, options.json, value) end
    self.models.ItemDate = function(value) return R.validate(V.check_ItemDate, options.json, value) end
    self.models.ItemGenre = function(value) return R.validate(V.check_ItemGenre, options.json, value) end
    self.models.Line = function(value) return R.validate(V.check_Line, options.json, value) end
    self.models.Lyrics = function(value) return R.validate(V.check_Lyrics, options.json, value) end
    self.models.LyricsList = function(value) return R.validate(V.check_LyricsList, options.json, value) end
    self.models.MediaType = function(value) return R.validate(V.check_MediaType, options.json, value) end
    self.models.MusicFolder = function(value) return R.validate(V.check_MusicFolder, options.json, value) end
    self.models.MusicFolders = function(value) return R.validate(V.check_MusicFolders, options.json, value) end
    self.models.OpenSubsonicExtension = function(value) return R.validate(V.check_OpenSubsonicExtension, options.json, value) end
    self.models.Playlist = function(value) return R.validate(V.check_Playlist, options.json, value) end
    self.models.PlaylistWithSongs = function(value) return R.validate(V.check_PlaylistWithSongs, options.json, value) end
    self.models.Playlists = function(value) return R.validate(V.check_Playlists, options.json, value) end
    self.models.RecordLabel = function(value) return R.validate(V.check_RecordLabel, options.json, value) end
    self.models.ReplayGain = function(value) return R.validate(V.check_ReplayGain, options.json, value) end
    self.models.ScanStatus = function(value) return R.validate(V.check_ScanStatus, options.json, value) end
    self.models.Search3Response = function(value) return R.validate(V.check_Search3Response, options.json, value) end
    self.models.SearchResult3 = function(value) return R.validate(V.check_SearchResult3, options.json, value) end
    self.models.Songs = function(value) return R.validate(V.check_Songs, options.json, value) end
    self.models.Starred2 = function(value) return R.validate(V.check_Starred2, options.json, value) end
    self.models.StructuredLyrics = function(value) return R.validate(V.check_StructuredLyrics, options.json, value) end
    self.models.SubsonicError = function(value) return R.validate(V.check_SubsonicError, options.json, value) end
    self.models.SubsonicResponse = function(value) return R.validate(V.check_SubsonicResponse, options.json, value) end
    self.operations.get_album = function(params) return O.get_album.new(options, params) end
    self.operations.get_album_list2 = function(params) return O.get_album_list2.new(options, params) end
    self.operations.get_artist = function(params) return O.get_artist.new(options, params) end
    self.operations.get_artists = function(params) return O.get_artists.new(options, params) end
    self.operations.get_cover_art = function(params) return O.get_cover_art.new(options, params) end
    self.operations.get_lyrics = function(params) return O.get_lyrics.new(options, params) end
    self.operations.get_lyrics_by_song_id = function(params) return O.get_lyrics_by_song_id.new(options, params) end
    self.operations.get_music_folders = function(params) return O.get_music_folders.new(options, params) end
    self.operations.get_open_subsonic_extensions = function(params) return O.get_open_subsonic_extensions.new(options, params) end
    self.operations.get_playlist = function(params) return O.get_playlist.new(options, params) end
    self.operations.get_playlists = function(params) return O.get_playlists.new(options, params) end
    self.operations.get_random_songs = function(params) return O.get_random_songs.new(options, params) end
    self.operations.get_scan_status = function(params) return O.get_scan_status.new(options, params) end
    self.operations.get_song = function(params) return O.get_song.new(options, params) end
    self.operations.get_starred2 = function(params) return O.get_starred2.new(options, params) end
    self.operations.ping = function(params) return O.ping.new(options, params) end
    self.operations.post_get_album = function(params) return O.post_get_album.new(options, params) end
    self.operations.post_get_album_list2 = function(params) return O.post_get_album_list2.new(options, params) end
    self.operations.post_get_artist = function(params) return O.post_get_artist.new(options, params) end
    self.operations.post_get_artists = function(params) return O.post_get_artists.new(options, params) end
    self.operations.post_get_lyrics = function(params) return O.post_get_lyrics.new(options, params) end
    self.operations.post_get_lyrics_by_song_id = function(params) return O.post_get_lyrics_by_song_id.new(options, params) end
    self.operations.post_get_music_folders = function(params) return O.post_get_music_folders.new(options, params) end
    self.operations.post_get_open_subsonic_extensions = function(params) return O.post_get_open_subsonic_extensions.new(options, params) end
    self.operations.post_get_playlist = function(params) return O.post_get_playlist.new(options, params) end
    self.operations.post_get_playlists = function(params) return O.post_get_playlists.new(options, params) end
    self.operations.post_get_random_songs = function(params) return O.post_get_random_songs.new(options, params) end
    self.operations.post_get_scan_status = function(params) return O.post_get_scan_status.new(options, params) end
    self.operations.post_get_song = function(params) return O.post_get_song.new(options, params) end
    self.operations.post_get_starred2 = function(params) return O.post_get_starred2.new(options, params) end
    self.operations.post_ping = function(params) return O.post_ping.new(options, params) end
    self.operations.post_search3 = function(params) return O.post_search3.new(options, params) end
    self.operations.post_star = function(params) return O.post_star.new(options, params) end
    self.operations.post_unstar = function(params) return O.post_unstar.new(options, params) end
    self.operations.search3 = function(params) return O.search3.new(options, params) end
    self.operations.star = function(params) return O.star.new(options, params) end
    self.operations.stream = function(params) return O.stream.new(options, params) end
    self.operations.unstar = function(params) return O.unstar.new(options, params) end
    return self
end

---@param params subsonic_api.get_albumParams
---@return subsonic_api.models.GetAlbumResponse
function Client:get_album(params)
    local operation = self.operations.get_album(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_album_list2Params
---@return subsonic_api.models.GetAlbumList2Response
function Client:get_album_list2(params)
    local operation = self.operations.get_album_list2(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_artistParams
---@return subsonic_api.models.GetArtistResponse
function Client:get_artist(params)
    local operation = self.operations.get_artist(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_artistsParams
---@return subsonic_api.models.GetArtistsResponse
function Client:get_artists(params)
    local operation = self.operations.get_artists(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_cover_artParams
---@return MluaHttpResponse
function Client:get_cover_art(params)
    local operation = self.operations.get_cover_art(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_lyricsParams
---@return subsonic_api.models.GetLyricsResponse
function Client:get_lyrics(params)
    local operation = self.operations.get_lyrics(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_lyrics_by_song_idParams
---@return subsonic_api.models.GetLyricsBySongIdResponse
function Client:get_lyrics_by_song_id(params)
    local operation = self.operations.get_lyrics_by_song_id(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_music_foldersParams
---@return subsonic_api.models.GetMusicFoldersResponse
function Client:get_music_folders(params)
    local operation = self.operations.get_music_folders(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_open_subsonic_extensionsParams
---@return subsonic_api.models.GetOpenSubsonicExtensionsResponse
function Client:get_open_subsonic_extensions(params)
    local operation = self.operations.get_open_subsonic_extensions(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_playlistParams
---@return subsonic_api.models.GetPlaylistResponse
function Client:get_playlist(params)
    local operation = self.operations.get_playlist(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_playlistsParams
---@return subsonic_api.models.GetPlaylistsResponse
function Client:get_playlists(params)
    local operation = self.operations.get_playlists(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_random_songsParams
---@return subsonic_api.models.GetRandomSongsResponse
function Client:get_random_songs(params)
    local operation = self.operations.get_random_songs(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_scan_statusParams
---@return subsonic_api.models.GetScanStatusResponse
function Client:get_scan_status(params)
    local operation = self.operations.get_scan_status(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_songParams
---@return subsonic_api.models.GetSongResponse
function Client:get_song(params)
    local operation = self.operations.get_song(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.get_starred2Params
---@return subsonic_api.models.GetStarred2Response
function Client:get_starred2(params)
    local operation = self.operations.get_starred2(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.pingParams
---@return subsonic_api.models.SubsonicResponse
function Client:ping(params)
    local operation = self.operations.ping(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_albumParams
---@return subsonic_api.models.GetAlbumResponse
function Client:post_get_album(params)
    local operation = self.operations.post_get_album(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_album_list2Params
---@return subsonic_api.models.GetAlbumList2Response
function Client:post_get_album_list2(params)
    local operation = self.operations.post_get_album_list2(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_artistParams
---@return subsonic_api.models.GetArtistResponse
function Client:post_get_artist(params)
    local operation = self.operations.post_get_artist(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_artistsParams
---@return subsonic_api.models.GetArtistsResponse
function Client:post_get_artists(params)
    local operation = self.operations.post_get_artists(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_lyricsParams
---@return subsonic_api.models.GetLyricsResponse
function Client:post_get_lyrics(params)
    local operation = self.operations.post_get_lyrics(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_lyrics_by_song_idParams
---@return subsonic_api.models.GetLyricsBySongIdResponse
function Client:post_get_lyrics_by_song_id(params)
    local operation = self.operations.post_get_lyrics_by_song_id(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_music_foldersParams
---@return subsonic_api.models.GetMusicFoldersResponse
function Client:post_get_music_folders(params)
    local operation = self.operations.post_get_music_folders(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_open_subsonic_extensionsParams
---@return subsonic_api.models.GetOpenSubsonicExtensionsResponse
function Client:post_get_open_subsonic_extensions(params)
    local operation = self.operations.post_get_open_subsonic_extensions(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_playlistParams
---@return subsonic_api.models.GetPlaylistResponse
function Client:post_get_playlist(params)
    local operation = self.operations.post_get_playlist(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_playlistsParams
---@return subsonic_api.models.GetPlaylistsResponse
function Client:post_get_playlists(params)
    local operation = self.operations.post_get_playlists(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_random_songsParams
---@return subsonic_api.models.GetRandomSongsResponse
function Client:post_get_random_songs(params)
    local operation = self.operations.post_get_random_songs(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_scan_statusParams
---@return subsonic_api.models.GetScanStatusResponse
function Client:post_get_scan_status(params)
    local operation = self.operations.post_get_scan_status(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_songParams
---@return subsonic_api.models.GetSongResponse
function Client:post_get_song(params)
    local operation = self.operations.post_get_song(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_get_starred2Params
---@return subsonic_api.models.GetStarred2Response
function Client:post_get_starred2(params)
    local operation = self.operations.post_get_starred2(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_pingParams
---@return subsonic_api.models.SubsonicResponse
function Client:post_ping(params)
    local operation = self.operations.post_ping(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_search3Params
---@return subsonic_api.models.Search3Response
function Client:post_search3(params)
    local operation = self.operations.post_search3(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_starParams
---@return subsonic_api.models.SubsonicResponse
function Client:post_star(params)
    local operation = self.operations.post_star(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.post_unstarParams
---@return subsonic_api.models.SubsonicResponse
function Client:post_unstar(params)
    local operation = self.operations.post_unstar(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.search3Params
---@return subsonic_api.models.Search3Response
function Client:search3(params)
    local operation = self.operations.search3(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.starParams
---@return subsonic_api.models.SubsonicResponse
function Client:star(params)
    local operation = self.operations.star(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.streamParams
---@return MluaHttpResponse
function Client:stream(params)
    local operation = self.operations.stream(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params subsonic_api.unstarParams
---@return subsonic_api.models.SubsonicResponse
function Client:unstar(params)
    local operation = self.operations.unstar(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

return Client
