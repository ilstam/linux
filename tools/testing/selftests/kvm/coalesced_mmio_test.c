// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Test the KVM_CREATE_COALESCED_MMIO_BUFFER, KVM_REGISTER_COALESCED_MMIO2 and
 * KVM_UNREGISTER_COALESCED_MMIO2 ioctls by making sure that MMIO writes to
 * associated zones end up in the correct ring buffer. Also test that we don't
 * exit to userspace when there is space in the corresponding buffer.
 */

/* #include <test_util.h> */
#include <time.h>
#include <kvm_util.h>


#define RING_SIZE(nentries) \
	(sizeof(struct kvm_coalesced_mmio_ring) + \
	 (nentries) * sizeof(struct kvm_coalesced_mmio))


static void guest_code(void)
{

}

static void test_create_bufer(struct kvm_vm *vm)
{
	int ring_fd;
	struct kvm_coalesced_mmio_ring *ring, *ring2;
	int r;

	ring_fd = __vm_ioctl(vm, KVM_CREATE_COALESCED_MMIO_BUFFER, NULL);

	/* Test with a random valid size  */
	unsigned int entries = rand() % 500 + 1;
	ring = mmap(NULL, RING_SIZE(entries), PROT_READ | PROT_WRITE,
		    MAP_PRIVATE, ring_fd, 0);
	TEST_ASSERT(ring != MAP_FAILED, "Failed to mmap() a buffer with %u entries",
		    entries);

	/* Test that trying to map the same fd again fails */
	ring2 = mmap(NULL, RING_SIZE(entries), PROT_READ | PROT_WRITE,
		    MAP_PRIVATE, ring_fd, 0);
	TEST_ASSERT(ring2 == MAP_FAILED && errno == EBUSY,
		    "Mapping the same fd again should fail with EBUSY");

	/* Test that munmap and close work */
	r = munmap(ring, RING_SIZE(entries));
	TEST_ASSERT(r == 0, "Failed to munmap()");
	r = close(ring_fd);
	TEST_ASSERT(r == 0, "Failed to close()");
}

// Test that first and last are zero at first

int main(int argc, char *argv[])
{
	struct kvm_vcpu *vcpu;
	struct kvm_vm *vm;
	int ring_fd;
	struct kvm_coalesced_mmio_ring *ring;
	int num_entries = 3;

	TEST_REQUIRE(kvm_has_cap(KVM_CAP_COALESCED_MMIO2));

	srand(time(NULL));

	vm = vm_create_with_one_vcpu(&vcpu, guest_code);

	test_create_bufer(vm);

	ring_fd = __vm_ioctl(vm, KVM_CREATE_COALESCED_MMIO_BUFFER, NULL);

	ring = mmap(NULL, RING_SIZE(num_entries), PROT_READ | PROT_WRITE,
		   MAP_PRIVATE, ring_fd, 0);
	TEST_ASSERT(ring != MAP_FAILED, "Failed to mmap() ring_fd");


	return 0;
}
