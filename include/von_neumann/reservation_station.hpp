#ifndef RESERVATION_STATION_HPP
#define RESERVATION_STATION_HPP

#include "von_neumann/uop.hpp"
#include <array>
#include <optional>

class ReservationStation {
public:
    static constexpr size_t RS_SIZE = 8;
    
    ReservationStation() = default;
    
    bool dispatch(const Uop& uop);
    void snoop_cdb(const CdbMessage& msg);
    std::optional<RsEntry> try_issue();
    std::optional<RsEntry> try_issue_oldest();
    bool is_full() const;
    void flush();

private:
    std::array<RsEntry, RS_SIZE> entries_{};
};

#endif