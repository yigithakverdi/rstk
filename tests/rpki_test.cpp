#include <gtest/gtest.h>

#include "engine/rpki/rpki.hpp"
#include "plugins/aspa/aspa.hpp"

class RPKITest : public ::testing::Test {
protected:
  std::shared_ptr<RPKI> rpki;

  void SetUp() override { rpki = std::make_shared<RPKI>(); }
};

TEST_F(RPKITest, BasicROAOperations) {
  // Test adding and querying ROAs
  rpki->ROAs[1].insert(2);
  EXPECT_TRUE(rpki->ROAs[1].find(2) != rpki->ROAs[1].end());
  EXPECT_FALSE(rpki->ROAs[1].find(3) != rpki->ROAs[1].end());
}

TEST_F(RPKITest, BasicUSPAOperations) {
  // Test ASPA object storage
  std::vector<int> providers = {2, 3};
  ASPAObject obj(1, providers, std::vector<unsigned char>{});
  rpki->USPAS[1] = obj;

  EXPECT_TRUE(rpki->USPAS.find(1) != rpki->USPAS.end());
  EXPECT_EQ(rpki->USPAS[1].providerASes, providers);
}
