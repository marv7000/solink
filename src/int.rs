use crate::elf::MachineType;

pub fn get_instructions(machine: MachineType, offset: u32) -> [u8; 16] {
    let mut result = [0u8; 16];
    match machine {
        MachineType::None => return result,
        MachineType::Amd64 => {
            // mov $offset
            result[0] = 0xe9;

            // Offset.
            let actual = (offset - 5).to_le_bytes();
            result[1] = actual[0];
            result[2] = actual[1];
            result[3] = actual[2];
            result[4] = actual[3];

            result[5] = 0x90;
            result[6] = 0x90;
            result[7] = 0x90;
            result[8] = 0x90;
            result[9] = 0x90;
            result[10] = 0x90;
            result[11] = 0x90;
            result[12] = 0x90;
            result[13] = 0x90;
            result[14] = 0x90;
            result[15] = 0x90;
        }
    }

    return result;
}
