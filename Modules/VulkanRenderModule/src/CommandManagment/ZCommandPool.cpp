#include "ZCommandPool.hpp"

using namespace CommandManagment;


bool ZCommandPool::initialize(uint32_t familyInde) {
	return false;
}
void ZCommandPool::shutdown() {

}
void ZCommandPool::reset() {

}
ZCommandBuffer ZCommandPool::getNewBuffer() {
	return ZCommandBuffer{};
}