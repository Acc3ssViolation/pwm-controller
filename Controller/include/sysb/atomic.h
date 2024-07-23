#ifndef ATOMIC_H_
#define ATOMIC_H_

// This disable a specific bit in a register and restores it after the statement following this has been executed.
#define NO_IRQ_BLOCK(reg, bit) for(uint8_t __NO_IRQ_BLOCK_RESTORE_MASK = reg & (1 << bit), __NO_IRQ_BLOCK_LOOP = 1; reg &= ~(__NO_IRQ_BLOCK_LOOP << bit), __NO_IRQ_BLOCK_LOOP; reg |= __NO_IRQ_BLOCK_RESTORE_MASK, __NO_IRQ_BLOCK_LOOP = 0)


#endif /* ATOMIC_H_ */