from simulation_parameters.sens_parameter import SensParameter


class TestSensParameter:
    def test_parse_and_serialize_sens_directive(self):
        # arrange
        directives = [
            ".SENS objfunc={V(2)} param=R1:R,C1:C",
            ".OPTIONS SENSITIVITY direct=0 adjoint=1",
            ".PREPROCESS REPLACEGROUND TRUE",
        ]
        # act
        params = SensParameter.from_xyce_directives(directives)
        # assert
        assert params is not None
        assert params.objective_mode == "objfunc"
        assert params.objective_values == ("V(2)",)
        assert params.parameter_list == ("R1:R", "C1:C")
        assert params.direct is False
        assert params.adjoint is True
        # act/assert
        assert params.to_xyce_directives() == [
            ".SENS objfunc={V(2)} param=R1:R,C1:C",
            ".OPTIONS SENSITIVITY direct=0 adjoint=1",
        ]
