local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")
local O = require("jellyfin_api.operations")

---@class jellyfin_api.Models
---@field AccessSchedule fun(value: jellyfin_api.models.AccessSchedule): jellyfin_api.models.AccessSchedule
---@field AudioSpatialFormat fun(value: jellyfin_api.models.AudioSpatialFormat): jellyfin_api.models.AudioSpatialFormat
---@field AuthenticateUserByName fun(value: jellyfin_api.models.AuthenticateUserByName): jellyfin_api.models.AuthenticateUserByName
---@field AuthenticationResult fun(value: jellyfin_api.models.AuthenticationResult): jellyfin_api.models.AuthenticationResult
---@field BaseItemDto fun(value: jellyfin_api.models.BaseItemDto): jellyfin_api.models.BaseItemDto
---@field BaseItemDtoQueryResult fun(value: jellyfin_api.models.BaseItemDtoQueryResult): jellyfin_api.models.BaseItemDtoQueryResult
---@field BaseItemKind fun(value: jellyfin_api.models.BaseItemKind): jellyfin_api.models.BaseItemKind
---@field BaseItemPerson fun(value: jellyfin_api.models.BaseItemPerson): jellyfin_api.models.BaseItemPerson
---@field ChannelType fun(value: jellyfin_api.models.ChannelType): jellyfin_api.models.ChannelType
---@field ChapterInfo fun(value: jellyfin_api.models.ChapterInfo): jellyfin_api.models.ChapterInfo
---@field ClientCapabilitiesDto fun(value: jellyfin_api.models.ClientCapabilitiesDto): jellyfin_api.models.ClientCapabilitiesDto
---@field CodecProfile fun(value: jellyfin_api.models.CodecProfile): jellyfin_api.models.CodecProfile
---@field CodecType fun(value: jellyfin_api.models.CodecType): jellyfin_api.models.CodecType
---@field CollectionType fun(value: jellyfin_api.models.CollectionType): jellyfin_api.models.CollectionType
---@field CollectionTypeOptions fun(value: jellyfin_api.models.CollectionTypeOptions): jellyfin_api.models.CollectionTypeOptions
---@field ContainerProfile fun(value: jellyfin_api.models.ContainerProfile): jellyfin_api.models.ContainerProfile
---@field DayOfWeek fun(value: jellyfin_api.models.DayOfWeek): jellyfin_api.models.DayOfWeek
---@field DeviceProfile fun(value: jellyfin_api.models.DeviceProfile): jellyfin_api.models.DeviceProfile
---@field DirectPlayProfile fun(value: jellyfin_api.models.DirectPlayProfile): jellyfin_api.models.DirectPlayProfile
---@field DlnaProfileType fun(value: jellyfin_api.models.DlnaProfileType): jellyfin_api.models.DlnaProfileType
---@field DynamicDayOfWeek fun(value: jellyfin_api.models.DynamicDayOfWeek): jellyfin_api.models.DynamicDayOfWeek
---@field EmbeddedSubtitleOptions fun(value: jellyfin_api.models.EmbeddedSubtitleOptions): jellyfin_api.models.EmbeddedSubtitleOptions
---@field EncodingContext fun(value: jellyfin_api.models.EncodingContext): jellyfin_api.models.EncodingContext
---@field EndPointInfo fun(value: jellyfin_api.models.EndPointInfo): jellyfin_api.models.EndPointInfo
---@field ExternalUrl fun(value: jellyfin_api.models.ExternalUrl): jellyfin_api.models.ExternalUrl
---@field ExtraType fun(value: jellyfin_api.models.ExtraType): jellyfin_api.models.ExtraType
---@field GeneralCommandType fun(value: jellyfin_api.models.GeneralCommandType): jellyfin_api.models.GeneralCommandType
---@field HardwareAccelerationType fun(value: jellyfin_api.models.HardwareAccelerationType): jellyfin_api.models.HardwareAccelerationType
---@field ImageFormat fun(value: jellyfin_api.models.ImageFormat): jellyfin_api.models.ImageFormat
---@field ImageOption fun(value: jellyfin_api.models.ImageOption): jellyfin_api.models.ImageOption
---@field ImageOrientation fun(value: jellyfin_api.models.ImageOrientation): jellyfin_api.models.ImageOrientation
---@field ImageType fun(value: jellyfin_api.models.ImageType): jellyfin_api.models.ImageType
---@field IsoType fun(value: jellyfin_api.models.IsoType): jellyfin_api.models.IsoType
---@field ItemFields fun(value: jellyfin_api.models.ItemFields): jellyfin_api.models.ItemFields
---@field ItemFilter fun(value: jellyfin_api.models.ItemFilter): jellyfin_api.models.ItemFilter
---@field ItemSortBy fun(value: jellyfin_api.models.ItemSortBy): jellyfin_api.models.ItemSortBy
---@field LibraryOptions fun(value: jellyfin_api.models.LibraryOptions): jellyfin_api.models.LibraryOptions
---@field LocationType fun(value: jellyfin_api.models.LocationType): jellyfin_api.models.LocationType
---@field LyricDto fun(value: jellyfin_api.models.LyricDto): jellyfin_api.models.LyricDto
---@field LyricLine fun(value: jellyfin_api.models.LyricLine): jellyfin_api.models.LyricLine
---@field LyricMetadata fun(value: jellyfin_api.models.LyricMetadata): jellyfin_api.models.LyricMetadata
---@field MediaAttachment fun(value: jellyfin_api.models.MediaAttachment): jellyfin_api.models.MediaAttachment
---@field MediaPathInfo fun(value: jellyfin_api.models.MediaPathInfo): jellyfin_api.models.MediaPathInfo
---@field MediaProtocol fun(value: jellyfin_api.models.MediaProtocol): jellyfin_api.models.MediaProtocol
---@field MediaSourceInfo fun(value: jellyfin_api.models.MediaSourceInfo): jellyfin_api.models.MediaSourceInfo
---@field MediaSourceType fun(value: jellyfin_api.models.MediaSourceType): jellyfin_api.models.MediaSourceType
---@field MediaStream fun(value: jellyfin_api.models.MediaStream): jellyfin_api.models.MediaStream
---@field MediaStreamProtocol fun(value: jellyfin_api.models.MediaStreamProtocol): jellyfin_api.models.MediaStreamProtocol
---@field MediaStreamType fun(value: jellyfin_api.models.MediaStreamType): jellyfin_api.models.MediaStreamType
---@field MediaType fun(value: jellyfin_api.models.MediaType): jellyfin_api.models.MediaType
---@field MediaUrl fun(value: jellyfin_api.models.MediaUrl): jellyfin_api.models.MediaUrl
---@field MetadataField fun(value: jellyfin_api.models.MetadataField): jellyfin_api.models.MetadataField
---@field NameGuidPair fun(value: jellyfin_api.models.NameGuidPair): jellyfin_api.models.NameGuidPair
---@field PersonKind fun(value: jellyfin_api.models.PersonKind): jellyfin_api.models.PersonKind
---@field PlayAccess fun(value: jellyfin_api.models.PlayAccess): jellyfin_api.models.PlayAccess
---@field PlayMethod fun(value: jellyfin_api.models.PlayMethod): jellyfin_api.models.PlayMethod
---@field PlaybackErrorCode fun(value: jellyfin_api.models.PlaybackErrorCode): jellyfin_api.models.PlaybackErrorCode
---@field PlaybackInfoDto fun(value: jellyfin_api.models.PlaybackInfoDto): jellyfin_api.models.PlaybackInfoDto
---@field PlaybackInfoResponse fun(value: jellyfin_api.models.PlaybackInfoResponse): jellyfin_api.models.PlaybackInfoResponse
---@field PlaybackOrder fun(value: jellyfin_api.models.PlaybackOrder): jellyfin_api.models.PlaybackOrder
---@field PlayerStateInfo fun(value: jellyfin_api.models.PlayerStateInfo): jellyfin_api.models.PlayerStateInfo
---@field ProblemDetails fun(value: jellyfin_api.models.ProblemDetails): jellyfin_api.models.ProblemDetails
---@field ProfileCondition fun(value: jellyfin_api.models.ProfileCondition): jellyfin_api.models.ProfileCondition
---@field ProfileConditionType fun(value: jellyfin_api.models.ProfileConditionType): jellyfin_api.models.ProfileConditionType
---@field ProfileConditionValue fun(value: jellyfin_api.models.ProfileConditionValue): jellyfin_api.models.ProfileConditionValue
---@field ProgramAudio fun(value: jellyfin_api.models.ProgramAudio): jellyfin_api.models.ProgramAudio
---@field QueueItem fun(value: jellyfin_api.models.QueueItem): jellyfin_api.models.QueueItem
---@field RepeatMode fun(value: jellyfin_api.models.RepeatMode): jellyfin_api.models.RepeatMode
---@field SeriesStatus fun(value: jellyfin_api.models.SeriesStatus): jellyfin_api.models.SeriesStatus
---@field SessionInfoDto fun(value: jellyfin_api.models.SessionInfoDto): jellyfin_api.models.SessionInfoDto
---@field SessionUserInfo fun(value: jellyfin_api.models.SessionUserInfo): jellyfin_api.models.SessionUserInfo
---@field SortOrder fun(value: jellyfin_api.models.SortOrder): jellyfin_api.models.SortOrder
---@field SubtitleDeliveryMethod fun(value: jellyfin_api.models.SubtitleDeliveryMethod): jellyfin_api.models.SubtitleDeliveryMethod
---@field SubtitlePlaybackMode fun(value: jellyfin_api.models.SubtitlePlaybackMode): jellyfin_api.models.SubtitlePlaybackMode
---@field SubtitleProfile fun(value: jellyfin_api.models.SubtitleProfile): jellyfin_api.models.SubtitleProfile
---@field SyncPlayUserAccessType fun(value: jellyfin_api.models.SyncPlayUserAccessType): jellyfin_api.models.SyncPlayUserAccessType
---@field TranscodeReason fun(value: jellyfin_api.models.TranscodeReason): jellyfin_api.models.TranscodeReason
---@field TranscodeSeekInfo fun(value: jellyfin_api.models.TranscodeSeekInfo): jellyfin_api.models.TranscodeSeekInfo
---@field TranscodingInfo fun(value: jellyfin_api.models.TranscodingInfo): jellyfin_api.models.TranscodingInfo
---@field TranscodingProfile fun(value: jellyfin_api.models.TranscodingProfile): jellyfin_api.models.TranscodingProfile
---@field TransportStreamTimestamp fun(value: jellyfin_api.models.TransportStreamTimestamp): jellyfin_api.models.TransportStreamTimestamp
---@field TrickplayInfo fun(value: jellyfin_api.models.TrickplayInfo): jellyfin_api.models.TrickplayInfo
---@field TypeOptions fun(value: jellyfin_api.models.TypeOptions): jellyfin_api.models.TypeOptions
---@field UnratedItem fun(value: jellyfin_api.models.UnratedItem): jellyfin_api.models.UnratedItem
---@field UserConfiguration fun(value: jellyfin_api.models.UserConfiguration): jellyfin_api.models.UserConfiguration
---@field UserDto fun(value: jellyfin_api.models.UserDto): jellyfin_api.models.UserDto
---@field UserItemDataDto fun(value: jellyfin_api.models.UserItemDataDto): jellyfin_api.models.UserItemDataDto
---@field UserPolicy fun(value: jellyfin_api.models.UserPolicy): jellyfin_api.models.UserPolicy
---@field Video3DFormat fun(value: jellyfin_api.models.Video3DFormat): jellyfin_api.models.Video3DFormat
---@field VideoRange fun(value: jellyfin_api.models.VideoRange): jellyfin_api.models.VideoRange
---@field VideoRangeType fun(value: jellyfin_api.models.VideoRangeType): jellyfin_api.models.VideoRangeType
---@field VideoType fun(value: jellyfin_api.models.VideoType): jellyfin_api.models.VideoType
---@field VirtualFolderInfo fun(value: jellyfin_api.models.VirtualFolderInfo): jellyfin_api.models.VirtualFolderInfo

---@class jellyfin_api.Operations
---@field authenticate_user_by_name fun(params: jellyfin_api.authenticate_user_by_nameParams): jellyfin_api.authenticate_user_by_nameOperation
---@field get_album_artists fun(params: jellyfin_api.get_album_artistsParams): jellyfin_api.get_album_artistsOperation
---@field get_artists fun(params: jellyfin_api.get_artistsParams): jellyfin_api.get_artistsOperation
---@field get_audio_stream fun(params: jellyfin_api.get_audio_streamParams): jellyfin_api.get_audio_streamOperation
---@field get_endpoint_info fun(params: jellyfin_api.get_endpoint_infoParams): jellyfin_api.get_endpoint_infoOperation
---@field get_item fun(params: jellyfin_api.get_itemParams): jellyfin_api.get_itemOperation
---@field get_item_image fun(params: jellyfin_api.get_item_imageParams): jellyfin_api.get_item_imageOperation
---@field get_items fun(params: jellyfin_api.get_itemsParams): jellyfin_api.get_itemsOperation
---@field get_lyrics fun(params: jellyfin_api.get_lyricsParams): jellyfin_api.get_lyricsOperation
---@field get_playlist_items fun(params: jellyfin_api.get_playlist_itemsParams): jellyfin_api.get_playlist_itemsOperation
---@field get_posted_playback_info fun(params: jellyfin_api.get_posted_playback_infoParams): jellyfin_api.get_posted_playback_infoOperation
---@field get_user_views fun(params: jellyfin_api.get_user_viewsParams): jellyfin_api.get_user_viewsOperation
---@field get_virtual_folders fun(params: jellyfin_api.get_virtual_foldersParams): jellyfin_api.get_virtual_foldersOperation
---@field mark_favorite_item fun(params: jellyfin_api.mark_favorite_itemParams): jellyfin_api.mark_favorite_itemOperation
---@field ping_playback_session fun(params: jellyfin_api.ping_playback_sessionParams): jellyfin_api.ping_playback_sessionOperation
---@field stop_encoding_process fun(params: jellyfin_api.stop_encoding_processParams): jellyfin_api.stop_encoding_processOperation
---@field unmark_favorite_item fun(params: jellyfin_api.unmark_favorite_itemParams): jellyfin_api.unmark_favorite_itemOperation

---@class jellyfin_api.Client
---@field operations jellyfin_api.Operations
---@field models jellyfin_api.Models
---@field private options jellyfin_api.Options
local Client = {}
Client.__index = Client

---@param options jellyfin_api.Options
---@return jellyfin_api.Client
function Client.new(options)
    R.options(options)
    local self = setmetatable({ options = options, models = {}, operations = {} }, Client)
    self.models.AccessSchedule = function(value) return R.validate(V.check_AccessSchedule, options.json, value) end
    self.models.AudioSpatialFormat = function(value) return R.validate(V.check_AudioSpatialFormat, options.json, value) end
    self.models.AuthenticateUserByName = function(value) return R.validate(V.check_AuthenticateUserByName, options.json, value) end
    self.models.AuthenticationResult = function(value) return R.validate(V.check_AuthenticationResult, options.json, value) end
    self.models.BaseItemDto = function(value) return R.validate(V.check_BaseItemDto, options.json, value) end
    self.models.BaseItemDtoQueryResult = function(value) return R.validate(V.check_BaseItemDtoQueryResult, options.json, value) end
    self.models.BaseItemKind = function(value) return R.validate(V.check_BaseItemKind, options.json, value) end
    self.models.BaseItemPerson = function(value) return R.validate(V.check_BaseItemPerson, options.json, value) end
    self.models.ChannelType = function(value) return R.validate(V.check_ChannelType, options.json, value) end
    self.models.ChapterInfo = function(value) return R.validate(V.check_ChapterInfo, options.json, value) end
    self.models.ClientCapabilitiesDto = function(value) return R.validate(V.check_ClientCapabilitiesDto, options.json, value) end
    self.models.CodecProfile = function(value) return R.validate(V.check_CodecProfile, options.json, value) end
    self.models.CodecType = function(value) return R.validate(V.check_CodecType, options.json, value) end
    self.models.CollectionType = function(value) return R.validate(V.check_CollectionType, options.json, value) end
    self.models.CollectionTypeOptions = function(value) return R.validate(V.check_CollectionTypeOptions, options.json, value) end
    self.models.ContainerProfile = function(value) return R.validate(V.check_ContainerProfile, options.json, value) end
    self.models.DayOfWeek = function(value) return R.validate(V.check_DayOfWeek, options.json, value) end
    self.models.DeviceProfile = function(value) return R.validate(V.check_DeviceProfile, options.json, value) end
    self.models.DirectPlayProfile = function(value) return R.validate(V.check_DirectPlayProfile, options.json, value) end
    self.models.DlnaProfileType = function(value) return R.validate(V.check_DlnaProfileType, options.json, value) end
    self.models.DynamicDayOfWeek = function(value) return R.validate(V.check_DynamicDayOfWeek, options.json, value) end
    self.models.EmbeddedSubtitleOptions = function(value) return R.validate(V.check_EmbeddedSubtitleOptions, options.json, value) end
    self.models.EncodingContext = function(value) return R.validate(V.check_EncodingContext, options.json, value) end
    self.models.EndPointInfo = function(value) return R.validate(V.check_EndPointInfo, options.json, value) end
    self.models.ExternalUrl = function(value) return R.validate(V.check_ExternalUrl, options.json, value) end
    self.models.ExtraType = function(value) return R.validate(V.check_ExtraType, options.json, value) end
    self.models.GeneralCommandType = function(value) return R.validate(V.check_GeneralCommandType, options.json, value) end
    self.models.HardwareAccelerationType = function(value) return R.validate(V.check_HardwareAccelerationType, options.json, value) end
    self.models.ImageFormat = function(value) return R.validate(V.check_ImageFormat, options.json, value) end
    self.models.ImageOption = function(value) return R.validate(V.check_ImageOption, options.json, value) end
    self.models.ImageOrientation = function(value) return R.validate(V.check_ImageOrientation, options.json, value) end
    self.models.ImageType = function(value) return R.validate(V.check_ImageType, options.json, value) end
    self.models.IsoType = function(value) return R.validate(V.check_IsoType, options.json, value) end
    self.models.ItemFields = function(value) return R.validate(V.check_ItemFields, options.json, value) end
    self.models.ItemFilter = function(value) return R.validate(V.check_ItemFilter, options.json, value) end
    self.models.ItemSortBy = function(value) return R.validate(V.check_ItemSortBy, options.json, value) end
    self.models.LibraryOptions = function(value) return R.validate(V.check_LibraryOptions, options.json, value) end
    self.models.LocationType = function(value) return R.validate(V.check_LocationType, options.json, value) end
    self.models.LyricDto = function(value) return R.validate(V.check_LyricDto, options.json, value) end
    self.models.LyricLine = function(value) return R.validate(V.check_LyricLine, options.json, value) end
    self.models.LyricMetadata = function(value) return R.validate(V.check_LyricMetadata, options.json, value) end
    self.models.MediaAttachment = function(value) return R.validate(V.check_MediaAttachment, options.json, value) end
    self.models.MediaPathInfo = function(value) return R.validate(V.check_MediaPathInfo, options.json, value) end
    self.models.MediaProtocol = function(value) return R.validate(V.check_MediaProtocol, options.json, value) end
    self.models.MediaSourceInfo = function(value) return R.validate(V.check_MediaSourceInfo, options.json, value) end
    self.models.MediaSourceType = function(value) return R.validate(V.check_MediaSourceType, options.json, value) end
    self.models.MediaStream = function(value) return R.validate(V.check_MediaStream, options.json, value) end
    self.models.MediaStreamProtocol = function(value) return R.validate(V.check_MediaStreamProtocol, options.json, value) end
    self.models.MediaStreamType = function(value) return R.validate(V.check_MediaStreamType, options.json, value) end
    self.models.MediaType = function(value) return R.validate(V.check_MediaType, options.json, value) end
    self.models.MediaUrl = function(value) return R.validate(V.check_MediaUrl, options.json, value) end
    self.models.MetadataField = function(value) return R.validate(V.check_MetadataField, options.json, value) end
    self.models.NameGuidPair = function(value) return R.validate(V.check_NameGuidPair, options.json, value) end
    self.models.PersonKind = function(value) return R.validate(V.check_PersonKind, options.json, value) end
    self.models.PlayAccess = function(value) return R.validate(V.check_PlayAccess, options.json, value) end
    self.models.PlayMethod = function(value) return R.validate(V.check_PlayMethod, options.json, value) end
    self.models.PlaybackErrorCode = function(value) return R.validate(V.check_PlaybackErrorCode, options.json, value) end
    self.models.PlaybackInfoDto = function(value) return R.validate(V.check_PlaybackInfoDto, options.json, value) end
    self.models.PlaybackInfoResponse = function(value) return R.validate(V.check_PlaybackInfoResponse, options.json, value) end
    self.models.PlaybackOrder = function(value) return R.validate(V.check_PlaybackOrder, options.json, value) end
    self.models.PlayerStateInfo = function(value) return R.validate(V.check_PlayerStateInfo, options.json, value) end
    self.models.ProblemDetails = function(value) return R.validate(V.check_ProblemDetails, options.json, value) end
    self.models.ProfileCondition = function(value) return R.validate(V.check_ProfileCondition, options.json, value) end
    self.models.ProfileConditionType = function(value) return R.validate(V.check_ProfileConditionType, options.json, value) end
    self.models.ProfileConditionValue = function(value) return R.validate(V.check_ProfileConditionValue, options.json, value) end
    self.models.ProgramAudio = function(value) return R.validate(V.check_ProgramAudio, options.json, value) end
    self.models.QueueItem = function(value) return R.validate(V.check_QueueItem, options.json, value) end
    self.models.RepeatMode = function(value) return R.validate(V.check_RepeatMode, options.json, value) end
    self.models.SeriesStatus = function(value) return R.validate(V.check_SeriesStatus, options.json, value) end
    self.models.SessionInfoDto = function(value) return R.validate(V.check_SessionInfoDto, options.json, value) end
    self.models.SessionUserInfo = function(value) return R.validate(V.check_SessionUserInfo, options.json, value) end
    self.models.SortOrder = function(value) return R.validate(V.check_SortOrder, options.json, value) end
    self.models.SubtitleDeliveryMethod = function(value) return R.validate(V.check_SubtitleDeliveryMethod, options.json, value) end
    self.models.SubtitlePlaybackMode = function(value) return R.validate(V.check_SubtitlePlaybackMode, options.json, value) end
    self.models.SubtitleProfile = function(value) return R.validate(V.check_SubtitleProfile, options.json, value) end
    self.models.SyncPlayUserAccessType = function(value) return R.validate(V.check_SyncPlayUserAccessType, options.json, value) end
    self.models.TranscodeReason = function(value) return R.validate(V.check_TranscodeReason, options.json, value) end
    self.models.TranscodeSeekInfo = function(value) return R.validate(V.check_TranscodeSeekInfo, options.json, value) end
    self.models.TranscodingInfo = function(value) return R.validate(V.check_TranscodingInfo, options.json, value) end
    self.models.TranscodingProfile = function(value) return R.validate(V.check_TranscodingProfile, options.json, value) end
    self.models.TransportStreamTimestamp = function(value) return R.validate(V.check_TransportStreamTimestamp, options.json, value) end
    self.models.TrickplayInfo = function(value) return R.validate(V.check_TrickplayInfo, options.json, value) end
    self.models.TypeOptions = function(value) return R.validate(V.check_TypeOptions, options.json, value) end
    self.models.UnratedItem = function(value) return R.validate(V.check_UnratedItem, options.json, value) end
    self.models.UserConfiguration = function(value) return R.validate(V.check_UserConfiguration, options.json, value) end
    self.models.UserDto = function(value) return R.validate(V.check_UserDto, options.json, value) end
    self.models.UserItemDataDto = function(value) return R.validate(V.check_UserItemDataDto, options.json, value) end
    self.models.UserPolicy = function(value) return R.validate(V.check_UserPolicy, options.json, value) end
    self.models.Video3DFormat = function(value) return R.validate(V.check_Video3DFormat, options.json, value) end
    self.models.VideoRange = function(value) return R.validate(V.check_VideoRange, options.json, value) end
    self.models.VideoRangeType = function(value) return R.validate(V.check_VideoRangeType, options.json, value) end
    self.models.VideoType = function(value) return R.validate(V.check_VideoType, options.json, value) end
    self.models.VirtualFolderInfo = function(value) return R.validate(V.check_VirtualFolderInfo, options.json, value) end
    self.operations.authenticate_user_by_name = function(params) return O.authenticate_user_by_name.new(options, params) end
    self.operations.get_album_artists = function(params) return O.get_album_artists.new(options, params) end
    self.operations.get_artists = function(params) return O.get_artists.new(options, params) end
    self.operations.get_audio_stream = function(params) return O.get_audio_stream.new(options, params) end
    self.operations.get_endpoint_info = function(params) return O.get_endpoint_info.new(options, params) end
    self.operations.get_item = function(params) return O.get_item.new(options, params) end
    self.operations.get_item_image = function(params) return O.get_item_image.new(options, params) end
    self.operations.get_items = function(params) return O.get_items.new(options, params) end
    self.operations.get_lyrics = function(params) return O.get_lyrics.new(options, params) end
    self.operations.get_playlist_items = function(params) return O.get_playlist_items.new(options, params) end
    self.operations.get_posted_playback_info = function(params) return O.get_posted_playback_info.new(options, params) end
    self.operations.get_user_views = function(params) return O.get_user_views.new(options, params) end
    self.operations.get_virtual_folders = function(params) return O.get_virtual_folders.new(options, params) end
    self.operations.mark_favorite_item = function(params) return O.mark_favorite_item.new(options, params) end
    self.operations.ping_playback_session = function(params) return O.ping_playback_session.new(options, params) end
    self.operations.stop_encoding_process = function(params) return O.stop_encoding_process.new(options, params) end
    self.operations.unmark_favorite_item = function(params) return O.unmark_favorite_item.new(options, params) end
    return self
end

---@param params jellyfin_api.authenticate_user_by_nameParams
---@return jellyfin_api.models.AuthenticationResult|jellyfin_api.models.AuthenticationResult|jellyfin_api.models.AuthenticationResult
function Client:authenticate_user_by_name(params)
    local operation = self.operations.authenticate_user_by_name(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_album_artistsParams
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function Client:get_album_artists(params)
    local operation = self.operations.get_album_artists(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_artistsParams
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function Client:get_artists(params)
    local operation = self.operations.get_artists(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_audio_streamParams
---@return MluaHttpResponse
function Client:get_audio_stream(params)
    local operation = self.operations.get_audio_stream(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_endpoint_infoParams
---@return jellyfin_api.models.EndPointInfo|jellyfin_api.models.EndPointInfo|jellyfin_api.models.EndPointInfo
function Client:get_endpoint_info(params)
    local operation = self.operations.get_endpoint_info(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_itemParams
---@return jellyfin_api.models.BaseItemDto|jellyfin_api.models.BaseItemDto|jellyfin_api.models.BaseItemDto
function Client:get_item(params)
    local operation = self.operations.get_item(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_item_imageParams
---@return MluaHttpResponse
function Client:get_item_image(params)
    local operation = self.operations.get_item_image(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_itemsParams
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function Client:get_items(params)
    local operation = self.operations.get_items(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_lyricsParams
---@return jellyfin_api.models.LyricDto|jellyfin_api.models.LyricDto|jellyfin_api.models.LyricDto
function Client:get_lyrics(params)
    local operation = self.operations.get_lyrics(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_playlist_itemsParams
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function Client:get_playlist_items(params)
    local operation = self.operations.get_playlist_items(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_posted_playback_infoParams
---@return jellyfin_api.models.PlaybackInfoResponse|jellyfin_api.models.PlaybackInfoResponse|jellyfin_api.models.PlaybackInfoResponse
function Client:get_posted_playback_info(params)
    local operation = self.operations.get_posted_playback_info(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_user_viewsParams
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function Client:get_user_views(params)
    local operation = self.operations.get_user_views(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.get_virtual_foldersParams
---@return jellyfin_api.models.Schema241|jellyfin_api.models.Schema242|jellyfin_api.models.Schema243
function Client:get_virtual_folders(params)
    local operation = self.operations.get_virtual_folders(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.mark_favorite_itemParams
---@return jellyfin_api.models.UserItemDataDto|jellyfin_api.models.UserItemDataDto|jellyfin_api.models.UserItemDataDto
function Client:mark_favorite_item(params)
    local operation = self.operations.mark_favorite_item(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.ping_playback_sessionParams
---@return nil
function Client:ping_playback_session(params)
    local operation = self.operations.ping_playback_session(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.stop_encoding_processParams
---@return nil
function Client:stop_encoding_process(params)
    local operation = self.operations.stop_encoding_process(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

---@param params jellyfin_api.unmark_favorite_itemParams
---@return jellyfin_api.models.UserItemDataDto|jellyfin_api.models.UserItemDataDto|jellyfin_api.models.UserItemDataDto
function Client:unmark_favorite_item(params)
    local operation = self.operations.unmark_favorite_item(params)
    local request = operation:build_request()
    return operation:parse_response(self.options.transport:send(request))
end

return Client
