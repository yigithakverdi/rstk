#include "engine/topology/topology.hpp"

class postop {
public:
  class config {
  public:
    virtual void configure(topology &t) = 0;
    virtual ~config();
  };

  class rpkiconfig : public config {
  public:
    void configure(topology &t) override;
  };

  class tierconfig : public config {
  public:
    void configure(topology &t) override;
  };

  class deploymentconfig : public config {
  public:
    void configure(topology &t) override;
  };
};
