#pragma once

#define GADGET_PROPERTY(_type_, _prop_, _var_, _val_) \
    Q_PROPERTY(_type_ _prop_ MEMBER _var_)            \
    _type_ _var_ { _val_ };

#define GADGET_PROPERTY_DEF(_type_, _prop_, _var_) GADGET_PROPERTY(_type_, _prop_, _var_, )

#define GATGET_LIST_PROPERTY(_type_, _prop_, _var_)                                   \
    Q_PROPERTY(QVariantList _prop_ READ get_##_prop_ WRITE set_##_prop_)              \
    QVariantList get_##_prop_() const { return QVariant::fromValue(_var_).toList(); } \
    void         set_##_prop_(const QVariantList& in) {                               \
        _var_.clear();                                                        \
        for (auto& el : in) {                                                 \
            _var_ << el.value<_type_>();                                      \
        }                                                                     \
    }                                                                                 \
    QList<_type_> _var_;

#define READ_PROPERTY(_type_, _prop_, _var_, _sig_)    \
    Q_PROPERTY(_type_ _prop_ READ _prop_ NOTIFY _sig_) \
    const auto& _prop_() const { return _var_; }       \
    _type_      _var_;

#define FORWARD_PROPERTY(_type_, _prop_, _input_)                                     \
public:                                                                               \
    Q_PROPERTY(_type_ _prop_ READ _prop_ WRITE set_##_prop_ NOTIFY changed_##_prop_)  \
    _type_ _prop_() const { return convert_from<_type_>(this->api().input._input_); } \
    void   set_##_prop_(_type_ v) {                                                   \
        auto& cur = this -> api().input._input_;                                    \
        auto  v_  = convert_from<std::decay_t<decltype(cur)>>(v);                   \
        if (cur != v_) {                                                            \
            cur = v_;                                                               \
            this->mark_dirty();                                                     \
            emit changed_##_prop_();                                                \
            this->reload_if_needed();                                               \
        }                                                                           \
    }                                                                                 \
Q_SIGNALS:                                                                            \
    void changed_##_prop_();

#define FORWARD_PROPERTY_DECLARE(_type_, _prop_, _input_)                            \
public:                                                                              \
    Q_PROPERTY(_type_ _prop_ READ _prop_ WRITE set_##_prop_ NOTIFY changed_##_prop_) \
    _type_ _prop_() const;                                                           \
    void   set_##_prop_(_type_ v);                                                   \
Q_SIGNALS:                                                                           \
    void changed_##_prop_();

#define FORWARD_PROPERTY_IMPL(_class_, _type_, _prop_, _input_)   \
    inline _type_ _class_::_prop_() const {                       \
        return convert_from<_type_>(this->api().input._input_);   \
    }                                                             \
    inline void _class_::set_##_prop_(_type_ v) {                 \
        auto& cur = this -> api().input._input_;                  \
        auto  v_  = convert_from<std::decay_t<decltype(cur)>>(v); \
        if (cur != v_) {                                          \
            cur = v_;                                             \
            this->mark_dirty();                                   \
            emit changed_##_prop_();                              \
            this->reload_if_needed();                             \
        }                                                         \
    }

#define DECLARE_PROPERTY(_type_, _prop_, _sig_)        \
    Q_PROPERTY(_type_ _prop_ READ _prop_ NOTIFY _sig_) \
    auto _prop_() const -> const _type_&;              \
    void set_##_prop_(const _type_&);

#define DECLARE_MODEL(...)