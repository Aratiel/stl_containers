// Copyright <jgerrick, kmodesta, lwolmer, tfelton> 2022
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include "s21_containers.h"
#include "s21_containersplus.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
