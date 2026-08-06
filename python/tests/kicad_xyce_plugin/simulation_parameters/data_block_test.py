from kicad_xyce_plugin.simulation_parameters.data_block import DataBlock


class TestDataBlockParsing:

    def test_parse_single_block_flattened(self):
        # arrange - .DATA block after _join_continuation_lines produces a flat directive
        directives = [
            ".DATA myTable r1 r2 1.0 2.0 3.0 4.0",
            ".ENDDATA",
        ]

        # act
        blocks = DataBlock.from_xyce_directives(directives)

        # assert
        assert len(blocks) == 1
        assert blocks[0].name == "myTable"
        assert blocks[0].parameters == ("r1", "r2")
        assert blocks[0].records == (("1.0", "2.0"), ("3.0", "4.0"))

    def test_parse_single_column_table(self):
        # arrange
        directives = [
            ".DATA temperatures TEMP 25.0 50.0 75.0",
            ".ENDDATA",
        ]

        # act
        blocks = DataBlock.from_xyce_directives(directives)

        # assert
        assert len(blocks) == 1
        assert blocks[0].parameters == ("TEMP",)
        assert blocks[0].records == (("25.0",), ("50.0",), ("75.0",))

    def test_parse_scientific_notation_values(self):
        # arrange
        directives = [
            ".DATA myTable r1 c1 8.0000e+00 4.0000e-09 9.0000e+00 4.0000e-09",
            ".ENDDATA",
        ]

        # act
        blocks = DataBlock.from_xyce_directives(directives)

        # assert
        assert len(blocks) == 1
        assert blocks[0].parameters == ("r1", "c1")
        assert blocks[0].records == (("8.0000e+00", "4.0000e-09"), ("9.0000e+00", "4.0000e-09"))

    def test_parse_multiple_blocks(self):
        # arrange
        directives = [
            ".DATA tableA r1 1.0 2.0",
            ".ENDDATA",
            ".DATA tableB c1 1e-9 2e-9",
            ".ENDDATA",
        ]

        # act
        blocks = DataBlock.from_xyce_directives(directives)

        # assert
        assert len(blocks) == 2
        assert blocks[0].name == "tableA"
        assert blocks[1].name == "tableB"

    def test_no_data_directives_returns_empty(self):
        # arrange
        directives = [".TRAN 1u 1m", ".STEP R1 1k 10k 1k"]

        # act
        blocks = DataBlock.from_xyce_directives(directives)

        # assert
        assert blocks == ()

    def test_spice_suffix_values_recognized(self):
        # arrange - values may use SPICE suffixes like k, meg, n
        directives = [
            ".DATA myTable r1 1k 2k 5k",
            ".ENDDATA",
        ]

        # act
        blocks = DataBlock.from_xyce_directives(directives)

        # assert
        assert len(blocks) == 1
        assert blocks[0].parameters == ("r1",)
        assert blocks[0].records == (("1k",), ("2k",), ("5k",))


class TestDataBlockGeneration:

    def test_generates_data_block_with_header_and_rows(self):
        # arrange
        block = DataBlock(name="myTable", parameters=("r1", "r2"), records=(("1.0", "2.0"), ("3.0", "4.0")))

        # act
        directives = block.to_xyce_directives()

        # assert
        assert directives[0] == ".DATA myTable"
        assert directives[1] == "+ r1 r2"
        assert directives[2] == "+ 1.0 2.0"
        assert directives[3] == "+ 3.0 4.0"
        assert directives[4] == ".ENDDATA"

    def test_generates_single_column_block(self):
        # arrange
        block = DataBlock(name="temps", parameters=("TEMP",), records=(("25.0",), ("75.0",)))

        # act
        directives = block.to_xyce_directives()

        # assert
        assert ".DATA temps" in directives
        assert "+ TEMP" in directives
        assert "+ 25.0" in directives
        assert ".ENDDATA" in directives

    def test_round_trip_preserves_structure(self):
        # arrange
        original = DataBlock(name="sweep", parameters=("r1", "c1"), records=(("1k", "10n"), ("2k", "20n")))

        # act - generate directives then parse them back
        directives = original.to_xyce_directives()
        parsed = DataBlock.from_xyce_directives(directives)

        # assert
        assert len(parsed) == 1
        assert parsed[0].name == original.name
        assert parsed[0].parameters == original.parameters
        assert parsed[0].records == original.records
