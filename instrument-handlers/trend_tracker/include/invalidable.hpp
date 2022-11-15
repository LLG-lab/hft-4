#ifndef __INVALIDABLE_HPP__
#define __INVALIDABLE_HPP__

class invalidable
{
public:

    invalidable(void)
        : valid_ {true}
    {}

    invalidable(bool valid)
        : valid_ {valid}
    {}


    virtual ~invalidable(void) {}

    void invalidate(void) { valid_ = false; }

protected:

    bool am_i_valid(void) const { return valid_; }

    void validate(void) { valid_ = true; }

private:

    bool valid_;
};

#endif /* __INVALIDABLE_HPP__ */
